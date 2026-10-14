///////////////////////////////////////////////////////////////////////////////
///
/// @file AudioSystem.cpp
/// @brief Unified audio system combining FMOD management and ECS integration
/// 
/// @Authors: Edwin Leow, Belinda Chua Jia Xuan 
/// @Copyright 2025, Digipen Institute of Technology
///
///////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "AudioSystem.h"
#include "Coordinator.h"
#include "TransformProperties.h"
#include "ImguiManager.h"
#include "AssetManager.h"
#include <iostream>
#include <cmath>
#include <filesystem>
#include <MessageSystem.h>
#include "EngineState.h"

// Global ECS interface
extern Framework::Coordinator ecsInterface;

namespace Framework {

    // Global AudioSystem pointer (replaces GlobalAudio)
    AudioSystem* g_AudioSystem = nullptr;

    //==========================================================================
    // Constructor / Destructor
    //==========================================================================

    AudioSystem::AudioSystem()
        : listenerPosition(0.0f),
        listenerForward(0.0f, 0.0f, -1.0f),
        listenerUp(0.0f, 1.0f, 0.0f),
        listenerVelocity(0.0f),
        lastListenerPosition(0.0f)
    {
        // Create FMOD Studio System
        FMOD_RESULT res = FMOD::Studio::System::create(&studioSystem);
        if (res != FMOD_OK || !studioSystem) {
            std::cerr << "[AudioSystem] Failed to create FMOD Studio system: " << res << std::endl;
            return;
        }

        // Initialize studio system
        res = studioSystem->initialize(512, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_3D_RIGHTHANDED, nullptr);
        if (res != FMOD_OK) {
            std::cerr << "[AudioSystem] Failed to initialize FMOD Studio system: " << res << std::endl;
            return;
        }

        // Get core system for DSP and low level features
        studioSystem->getCoreSystem(&coreSystem);

        // Configure 3D default settings
        Configure3DSettings(current3DSettings);

        // Initialize DSP effects
        InitializeReverb();

        // Set global pointer
        g_AudioSystem = this;
    }

    Framework::AudioSystem::~AudioSystem() {
        // 1. Stop all events safely
        for (auto& pair : activeInstances) {
            if (pair.second && pair.second->isValid()) {
                pair.second->stop(FMOD_STUDIO_STOP_IMMEDIATE);
            }
        }
        activeInstances.clear();

        for (auto& p : activeEventsById) {
            if (p.second && p.second->isValid()) {
                p.second->stop(FMOD_STUDIO_STOP_IMMEDIATE);
            }
        }
        activeEventsById.clear();

        for (auto& p : activeEvents) {
            if (p.second && p.second->isValid()) {
                p.second->stop(FMOD_STUDIO_STOP_IMMEDIATE);
            }
        }
        activeEvents.clear();

        loadedEvents.clear();

        // 2. Release DSPs (These are attached to Core, release them before Core dies)
        // Clear all Banks
        for (auto& [path, bank] : loadedBanks)
        {
            if (bank)
                bank->unload();
            bank->unloadSampleData();
        }
        loadedBanks.clear();

        // Clean up DSP Effects
        if (reverbDSP) {
            reverbDSP->release();
            reverbDSP = nullptr;
        }
        if (lowPassFilter) {
            lowPassFilter->release();
            lowPassFilter = nullptr;
        }

        if (studioSystem) {
            // 3. CRITICAL: Flush the command queue!
            // This ensures the "Stop" commands above actually happen 
            // before we rip the carpet out from under the audio driver.
            studioSystem->update();

            // 4. Unload banks
            studioSystem->unloadAll();

            // 5. Release the System
            // This will automatically clean up any lingering event instances
            studioSystem->release();
            studioSystem = nullptr;
        }

        coreSystem = nullptr;
        g_AudioSystem = nullptr;
    }

    //==========================================================================
    // ISystem Interface Implementation
    //==========================================================================

    void AudioSystem::Initialize() {
        std::cout << "[AudioSystem] Initializing..." << std::endl;
        ImguiManager::Log("Initializing...", "AudioSystem");

        // Register AudioComponent
        std::cout << "[AudioSystem] Registering AudioComponent..." << std::endl;
        ImguiManager::Log("Registering AudioComponent...", "AudioSystem");
        ecsInterface.RegisterComponent<AudioComponent>();
        std::cout << "[AudioSystem] AudioComponent registered" << std::endl;
        ImguiManager::LogSuccess("AudioComponent registered", "AudioSystem");

        // Check if TransformComponent is already registered
        std::cout << "[AudioSystem] Checking if TransformComponent is already registered..." << std::endl;
        try {
            (void)ecsInterface.GetComponentType<TransformComponent>();
            std::cout << "[AudioSystem] TransformComponent already registered by another system (OK)" << std::endl;
        }
        catch (...) {
            std::cout << "[AudioSystem] TransformComponent not registered, registering now..." << std::endl;
            ecsInterface.RegisterComponent<TransformComponent>();
            std::cout << "[AudioSystem] TransformComponent registered" << std::endl;
        }

        // Set signature
        Signature sig;
        sig.set(ecsInterface.GetComponentType<AudioComponent>(), true);
        sig.set(ecsInterface.GetComponentType<TransformComponent>(), true);
        ecsInterface.SetSystemSignature<AudioSystem>(sig);

        std::stringstream ss;
        ss << sig;
        std::cout << "[AudioSystem] Signature set: " << sig << std::endl;
        ImguiManager::Log("Signature set: " + ss.str(), "AudioSystem");

        // Subscribe to messages
        MessageBus::Get().Subscribe<EntityDestroyedMessage>(
            [this](const EntityDestroyedMessage& msg) { OnEntityDestroyed(msg); });

        MessageBus::Get().Subscribe<PlaySoundMessage>(
            [this](const PlaySoundMessage& msg) { OnPlaySound(msg); });

        MessageBus::Get().Subscribe<EnterPlayModeMessage>(
            [this](const EnterPlayModeMessage& msg) {
                (void)msg;
                OnEnterPlayMode();
            });

        MessageBus::Get().Subscribe<ExitPlayModeMessage>(
            [this](const ExitPlayModeMessage& msg) {
                (void)msg;
                OnExitPlayMode();
            });

        MessageBus::Get().Subscribe<PausePlayModeMessage>(
            [this](const PausePlayModeMessage& msg) {
                (void)msg;
                OnPausedPlayMode();
            });

        MessageBus::Get().Subscribe<ResumePlayModeMessage>(
            [this](const ResumePlayModeMessage& msg) {
                (void)msg;
                OnResumePlayMode();
            });

        std::cout << "[AudioSystem] Initialized with 3D audio support" << std::endl;
        ImguiManager::LogSuccess("Initialized with 3D audio support", "AudioSystem");

    }

    void AudioSystem::Update(float deltaTime) {

        
        Entity activeListenerEntity = {};
        bool foundListener = false;

        // Find active listener entity
        for (Entity e : mEntities) {
            if (!ecsInterface.HasComponent<ListenerComponent>(e)) continue;
            if (!ecsInterface.HasComponent<TransformComponent>(e)) continue;

            auto& listener = ecsInterface.GetComponent<ListenerComponent>(e);
            if (!listener.active) continue;

            activeListenerEntity = e;
            foundListener = true;
            break;
        }

        // If no listener found, bail or use fallback
        if (!foundListener) {
            FMOD_3D_ATTRIBUTES fallbackAttribs = {};
            fallbackAttribs.position = { 0.0f, 0.0f, 0.0f };
            fallbackAttribs.forward = { 0.0f, 0.0f, -1.0f };
            fallbackAttribs.up = { 0.0f, 1.0f, 0.0f };
            fallbackAttribs.velocity = { 0.0f, 0.0f, 0.0f };

            if (studioSystem) {
                studioSystem->setListenerAttributes(0, &fallbackAttribs);
                studioSystem->update();
            }
            return;
        }

        // Update listener attributes from ECS
        auto& listener = ecsInterface.GetComponent<ListenerComponent>(activeListenerEntity);
        auto& transform = ecsInterface.GetComponent<TransformComponent>(activeListenerEntity);

        // Handle fade effects for smooth BGM transitions
        if (isFadingIn || isFadingOut) {
            fadeTimer += deltaTime;
            float progress = fadeTimer / fadeDuration;
            progress = std::min(1.0f, std::max(0.0f, progress));

            if (isFadingIn) {
                float currentVolume = fadeStartVolume + (fadeTargetVolume - fadeStartVolume) * progress;
                if (currentBGMChannel >= 0 && activeEventsById.count(currentBGMChannel)) {
                    activeEventsById[currentBGMChannel]->setVolume(currentVolume * bgmVolume * masterVolume);
                }

                if (progress >= 1.0f) {
                    isFadingIn = false;
                    std::cout << "[AudioSystem] Fade in complete" << std::endl;
                }
            }
            else if (isFadingOut) {
                float currentVolume = fadeStartVolume * (1.0f - progress);
                if (currentBGMChannel >= 0 && activeEventsById.count(currentBGMChannel)) {
                    activeEventsById[currentBGMChannel]->setVolume(currentVolume * bgmVolume * masterVolume);
                }

                if (progress >= 1.0f) {
                    isFadingOut = false;
                    if (currentBGMChannel >= 0) {
                        StopChannel(currentBGMChannel);
                    }

                    if (pendingBGMChannel >= 0) {
                        currentBGMChannel = pendingBGMChannel;
                        pendingBGMChannel = -1;
                        FadeBGMIn(fadeDuration);
                    }
                }
            }
        }

        // Update listener velocity for Doppler effect
        if (deltaTime > 0.0f) listener.velocity = (transform.position - listener.position) / deltaTime;
        listener.position = transform.position;

        // Update FMOD listener attributes
        FMOD_3D_ATTRIBUTES listenerAttribs{};
        listenerAttribs.position = { listener.position.x,  listener.position.y,    listener.position.z };
        listenerAttribs.forward = { listener.forward.x,   listener.forward.y,     listener.forward.z };
        listenerAttribs.up = { listener.up.x,        listener.up.y,          listener.up.z };
        listenerAttribs.velocity = { listener.velocity.x,  listener.velocity.y,    listener.velocity.z };

        if (studioSystem) studioSystem->setListenerAttributes(listener.listenerIndex, &listenerAttribs);

        // Update all entity audio sources
        for (Entity entity : mEntities)
        {
            if (!ecsInterface.HasComponent<AudioComponent>(entity)) continue;
            if (!ecsInterface.HasComponent<TransformComponent>(entity)) continue;

            auto& audioComp = ecsInterface.GetComponent<AudioComponent>(entity);
            auto& transform = ecsInterface.GetComponent<TransformComponent>(entity);

            if (audioComp.is3D && audioComp.isPlaying && audioComp.eventInstance)
            {
                glm::vec3 velocity(0.0f);
                if (deltaTime > 0.0f) velocity = (transform.position - audioComp.lastPosition) / deltaTime;

                Update3DAttributes(entity, transform.position, velocity);
                audioComp.lastPosition = transform.position;
            }
        }

        if (!engineState.HasState(EngineMode::Play) && !audioPaused)
        {
            PauseAllAudio();
            audioPaused = true;
        }
        else if (engineState.HasState(EngineMode::Play) && audioPaused)
        {
            ResumeAllAudio();
            audioPaused = false;
        }

        // Update FMOD
        if (studioSystem) studioSystem->update();
    }

    //==========================================================================
    // FMOD System Management
    //==========================================================================

    void AudioSystem::InitializeFMOD() {
        std::cout << "Current working directory: " << std::filesystem::current_path() << std::endl;
        std::cout << "[AudioSystem] Initializing FMOD with 3D support." << std::endl;

        // Get FMOD version
        unsigned int version = 0;
        if (studioSystem) {
            FMOD::System* core = nullptr;
            studioSystem->getCoreSystem(&core);
            if (core) {
                core->getVersion(&version);
                std::cout << "[AudioSystem] FMOD Engine version: "
                    << ((version >> 16) & 0xFFFF) << "."
                    << ((version >> 8) & 0xFF) << "."
                    << (version & 0xFF) << std::endl;
            }
        }

        std::filesystem::path root = GetEngineRootPath();
        std::string audioBinPath = (root / "Resource/AudioBank.bin").string();
        //std::cout << "  Loading: " << audioBinPath << std::endl;
        AssetManager::Instance().LoadBinaryArray<AudioBankMetadata>(audioBinPath, g_AudioBankMetadataProps, [](AudioBankMetadata& meta)
            {
                // Load the bank into FMOD
                FMOD::Studio::Bank* bank = g_AudioSystem->LoadBank(meta.path);
                if (!bank) return;

                // Query events
                int eventCount = 0;
                bank->getEventCount(&eventCount);

                std::vector<FMOD::Studio::EventDescription*> eventList(eventCount);
                bank->getEventList(eventList.data(), eventCount, &eventCount);

                meta.events.clear();
                //std::cout << "[AudioSystem] Bank '" << meta.name << "' contains " << eventCount << " events" << std::endl;

                for (auto* ev : eventList)
                {
                    char path[256];
                    int retrieved = 0;
                    ev->getPath(path, sizeof(path), &retrieved);
                    meta.events.push_back(std::string(path));
                    //std::cout << "   -> Event: " << path << std::endl;
                }
            }
        );

        std::cout << "\n[AudioSystem] ========================================" << std::endl;
        std::cout << "[AudioSystem] All bank files and sample data loaded!" << std::endl;
        std::cout << "[AudioSystem] ========================================\n" << std::endl;

        // Create references to common buses
        CreateBus("bus:/");
        CreateBus("bus:/BGM");
        CreateBus("bus:/SFX");
        CreateBus("bus:/Ambient");
        
    }

    void AudioSystem::PrintAudioInfo() const {
        std::cout << "\n=== Audio System Info ===" << std::endl;
        std::cout << "Master Volume: " << masterVolume << std::endl;
        std::cout << "BGM Volume: " << bgmVolume << std::endl;
        std::cout << "SFX Volume: " << sfxVolume << std::endl;
        std::cout << "Loaded Events: " << loadedEvents.size() << std::endl;
        std::cout << "Active Events: " << activeEvents.size() << std::endl;
        std::cout << "Active Entity Instances: " << activeInstances.size() << std::endl;
        std::cout << "=========================\n" << std::endl;
    }

    FMOD::Studio::Bank* AudioSystem::LoadBank(const std::string& bankPath)
    {
        auto it = loadedBanks.find(bankPath);
        if (it != loadedBanks.end())
        {
            //std::cout << "[AudioSystem] Using cached bank: " << bankPath << std::endl;
            return it->second;
        }

        FMOD::Studio::Bank* bank = nullptr;

        // loadBankFile only loads the metadata, but does not load the actual audio sample data
        // Which means event instances can be created but without sample data, audio would not be played
        FMOD_RESULT result = studioSystem->loadBankFile(bankPath.c_str(), FMOD_STUDIO_LOAD_BANK_NORMAL, &bank);

        if (result != FMOD_OK || !bank)
        {
            std::cerr << "[AudioSystem] ERROR: Failed to load bank: " << bankPath << " (error: " << result << ")" << std::endl;
            return nullptr;
        }
        //std::cout << "[AudioSystem] Bank loaded: " << bankPath << std::endl;

        // Load sample data so events can actually play
        result = bank->loadSampleData();
        if (result != FMOD_OK)
        {
            std::cerr << "[AudioSystem] ERROR: Failed to load sample data for bank: " << bankPath << " (error: " << result << ")" << std::endl;
        }
        else
        {
            //std::cout << "[AudioSystem] Sample data loaded for bank: " << bankPath << std::endl;
        }
        loadedBanks[bankPath] = bank;
        return bank;
    }

    //==========================================================================
    // Event Loading and Playback
    //==========================================================================

    FMOD_MODE AudioSystem::GetModeFromString(const std::string& mode) {
        if (mode == "loop") return FMOD_LOOP_NORMAL;
        if (mode == "stream") return FMOD_CREATESTREAM;
        return FMOD_DEFAULT;
    }

    AudioSystem::SoundType AudioSystem::ParseSoundType(const std::string& str) {
        if (str == "BGM" || str == "Music") return BACKGROUND_MUSIC;
        if (str == "SFX" || str == "Effect") return SOUND_EFFECT;
        return EMPTY;
    }

    FMOD::Studio::EventDescription* AudioSystem::LoadEvent(const std::string& eventPath) {
        std::cout << "[AudioSystem] LoadEvent called for: " << eventPath << std::endl;

        // Check cache first
        auto it = loadedEvents.find(eventPath);
        if (it != loadedEvents.end()) {
            std::cout << "[AudioSystem] Using cached event: " << eventPath << std::endl;
            return it->second;
        }

        // Determine the path to use
        std::string pathToUse = eventPath;
        if (eventPath.find("event:/") != 0) {
            pathToUse = "event:/" + eventPath;
            std::cout << "[AudioSystem] Prepended event:/ -> " << pathToUse << std::endl;
        }
        else {
            std::cout << "[AudioSystem] Using provided path: " << pathToUse << std::endl;
        }

        // Load from FMOD
        FMOD::Studio::EventDescription* eventDesc = nullptr;
        FMOD_RESULT result = studioSystem->getEvent(pathToUse.c_str(), &eventDesc);

        if (result != FMOD_OK || !eventDesc) {
            std::cerr << "[AudioSystem] Failed to load event: " << pathToUse << " (error: " << result << ")" << std::endl;
            return nullptr;
        }

        // Cache and return
        loadedEvents[eventPath] = eventDesc;
        std::cout << "[AudioSystem] Event loaded and cached: " << eventPath << std::endl;
        return eventDesc;
    }

    void AudioSystem::PlayAudio(const std::string& eventPath) {
        std::cout << "[AudioSystem] PlayAudio called with: " << eventPath << std::endl;

        FMOD::Studio::EventDescription* eventDesc = LoadEvent(eventPath);
        if (!eventDesc) {
            std::cerr << "[AudioSystem] PlayAudio failed - could not load: " << eventPath << std::endl;
            return;
        }

        FMOD::Studio::EventInstance* instance = nullptr;
        FMOD_RESULT result = eventDesc->createInstance(&instance);

        if (result != FMOD_OK || !instance) {
            std::cerr << "[AudioSystem] Failed to create instance for: " << eventPath << std::endl;
            return;
        }

        // Check if 3D and set position at listener
        bool is3D = false;
        eventDesc->is3D(&is3D);
        if (is3D) {
            FMOD_3D_ATTRIBUTES attributes = {};
            attributes.position = { listenerPosition.x, listenerPosition.y, listenerPosition.z };
            attributes.velocity = { 0.0f, 0.0f, 0.0f };
            attributes.forward = { 0.0f, 0.0f, 1.0f };
            attributes.up = { 0.0f, 1.0f, 0.0f };
            instance->set3DAttributes(&attributes);
            std::cout << "[AudioSystem] Set 3D position at listener: (" << listenerPosition.x << ", " << listenerPosition.y << ", " << listenerPosition.z << ")" << std::endl;
        }

        instance->setVolume(masterVolume);
        result = instance->start();

        if (result != FMOD_OK)
        {
            std::cerr << "[AudioSystem] Failed to start instance: " << result << std::endl;
            instance->release();
            return;
        }

        // Stop and release old instance if one exists for this event
        auto oldIt = activeEvents.find(eventPath);
        if (oldIt != activeEvents.end() && oldIt->second) {
            FMOD_STUDIO_PLAYBACK_STATE oldState;
            oldIt->second->getPlaybackState(&oldState);
            if (oldState != FMOD_STUDIO_PLAYBACK_STOPPED) {
                oldIt->second->stop(FMOD_STUDIO_STOP_IMMEDIATE);
            }
            oldIt->second->release();
        }

        // Store in active events
        activeEvents[eventPath] = instance;
        int channelId = GetNextChannelId();
        activeEventsById[channelId] = instance;

        std::cout << "[AudioSystem] Playing: " << eventPath << " (3D: " << (is3D ? "Yes" : "No") << ")" << std::endl;

        // Check playback state after a few updates
        for (int i = 0; i < 3; i++) {
            if (studioSystem) studioSystem->update();
        }

        FMOD_STUDIO_PLAYBACK_STATE state;
        instance->getPlaybackState(&state);
        std::cout << "[AudioSystem] Playback state after start: " << state << std::endl;
    }

    // Add this function anywhere in AudioSystem.cpp (suggest after the PlayAudio function)

    void AudioSystem::StopEvent(const std::string& eventName)
    {
        // Check if the event is currently active
        auto it = activeEvents.find(eventName);
        if (it != activeEvents.end() && it->second) {
            FMOD::Studio::EventInstance* instance = it->second;

            // Stop the event (use ALLOWFADEOUT for smooth stop, IMMEDIATE for instant)
            FMOD_RESULT result = instance->stop(FMOD_STUDIO_STOP_ALLOWFADEOUT);

            if (result == FMOD_OK) {
                std::cout << "[AudioSystem] Stopped event: " << eventName << std::endl;
            }
            else {
                std::cerr << "[AudioSystem] Failed to stop event: " << eventName
                    << " Error: " << result << std::endl;
            }

            // Release the instance
            instance->release();

            // Remove from active events map
            activeEvents.erase(it);
        }
        else {
            std::cerr << "[AudioSystem] Event not found or not playing: " << eventName << std::endl;
        }
    }

    void AudioSystem::PauseSound(const std::string& customName) {
        auto it = activeEvents.find(customName);
        if (it != activeEvents.end() && it->second) {
            it->second->setPaused(true);
            std::cout << "[AudioSystem] Paused: " << customName << std::endl;
        }
        else {
            std::cerr << "[AudioSystem] Sound not found: " << customName << std::endl;
        }
    }

    void AudioSystem::SetVolume(const std::string& customName, float volume) {
        auto it = activeEvents.find(customName);
        if (it != activeEvents.end() && it->second) {
            it->second->setVolume(volume);
            std::cout << "[AudioSystem] Volume for " << customName << " set to " << volume << std::endl;
        }
        else {
            std::cerr << "[AudioSystem] Sound not found: " << customName << std::endl;
        }
    }

    void AudioSystem::StopChannel(int channelId) {
        auto it = activeEventsById.find(channelId);
        if (it != activeEventsById.end() && it->second) {
            it->second->stop(FMOD_STUDIO_STOP_IMMEDIATE);
            it->second->release();
            activeEventsById.erase(it);
            std::cout << "[AudioSystem] Stopped channel: " << channelId << std::endl;
        }
    }

    bool AudioSystem::IsPlaying(const std::string& eventPath) const
    {
        auto it = activeEvents.find(eventPath);
        if (it == activeEvents.end() || !it->second) return false;

        FMOD_STUDIO_PLAYBACK_STATE state;
        it->second->getPlaybackState(&state);
        return state == FMOD_STUDIO_PLAYBACK_PLAYING;
    }

    bool AudioSystem::IsPaused(const std::string& eventPath) const
    {
        auto it = activeEvents.find(eventPath);
        if (it == activeEvents.end() || !it->second) return false;

        bool paused = false;
        it->second->getPaused(&paused);
        return paused;
    }

    FMOD::Studio::EventInstance* AudioSystem::GetEventInstance(const std::string& eventPath) const
    {
        auto it = activeEvents.find(eventPath);
        if (it != activeEvents.end()) return it->second;
        return nullptr;
    }

    //==========================================================================
    // 3D Audio Functions
    //==========================================================================

    int AudioSystem::PlaySound3D(
        const std::string& customName,
        const Vector3& position,
        const Vector3& velocity,
        float volume,
        AudioSourceType sourceType)
    {
        FMOD::Studio::EventDescription* eventDesc = LoadEvent(customName);
        if (!eventDesc) return -1;

        FMOD::Studio::EventInstance* instance = nullptr;
        if (eventDesc->createInstance(&instance) != FMOD_OK || !instance) {
            return -1;
        }

        // Set 3D attributes
        FMOD_3D_ATTRIBUTES attributes = {};
        attributes.position = VectorToFmod(position);
        attributes.velocity = VectorToFmod(velocity);
        attributes.forward = { 0.0f, 0.0f, 1.0f };
        attributes.up = { 0.0f, 1.0f, 0.0f };
        instance->set3DAttributes(&attributes);

        instance->setVolume(volume * masterVolume);
        instance->start();

        // Auto-release one-shot (non-looping) events after playback ends
        bool isOneshot = false;
        eventDesc->isOneshot(&isOneshot);
        if (isOneshot) {
            instance->release();
            // Don't track released instances - they auto-clean
            // Still need a channelId for the return value
            int channelId = GetNextChannelId();

            // Track 3D info
            Channel3DInfo info;
            info.position = position;
            info.velocity = velocity;
            info.baseVolume = volume;
            info.type = sourceType;
            channel3DInfo[channelId] = info;

            return channelId;
        }

        // Looping event: stop old instance if one exists with this name
        auto oldIt = activeEvents.find(customName);
        if (oldIt != activeEvents.end() && oldIt->second) {
            oldIt->second->stop(FMOD_STUDIO_STOP_IMMEDIATE);
            oldIt->second->release();
        }
        activeEvents[customName] = instance;

        int channelId = GetNextChannelId();
        activeEventsById[channelId] = instance;

        // Track 3D info
        Channel3DInfo info;
        info.position = position;
        info.velocity = velocity;
        info.baseVolume = volume;
        info.type = sourceType;
        channel3DInfo[channelId] = info;

        return channelId;
    }

    void AudioSystem::Set3DListener(
        const Vector3& position,
        const Vector3& velocity,
        const Vector3& forward,
        const Vector3& up)
    {
        listenerPosition = glm::vec3(position.x, position.y, position.z);
        listenerForward = glm::vec3(forward.x, forward.y, forward.z);
        listenerUp = glm::vec3(up.x, up.y, up.z);

        FMOD_3D_ATTRIBUTES attributes = {};
        attributes.position = VectorToFmod(position);
        attributes.velocity = VectorToFmod(velocity);
        attributes.forward = VectorToFmod(forward);
        attributes.up = VectorToFmod(up);

        if (studioSystem) {
            studioSystem->setListenerAttributes(0, &attributes);
        }
    }

    void AudioSystem::Set3DListenerMultiple(
        int listenerId,
        const Vector3& position,
        const Vector3& velocity,
        const Vector3& forward,
        const Vector3& up)
    {
        FMOD_3D_ATTRIBUTES attributes = {};
        attributes.position = VectorToFmod(position);
        attributes.velocity = VectorToFmod(velocity);
        attributes.forward = VectorToFmod(forward);
        attributes.up = VectorToFmod(up);

        if (studioSystem) {
            studioSystem->setListenerAttributes(listenerId, &attributes);
        }
    }

    void AudioSystem::UpdateChannel3DAttributes(
        int channelId,
        const Vector3& position,
        const Vector3& velocity)
    {
        auto it = activeEventsById.find(channelId);
        if (it != activeEventsById.end() && it->second) {
            FMOD_3D_ATTRIBUTES attributes = {};
            attributes.position = VectorToFmod(position);
            attributes.velocity = VectorToFmod(velocity);
            attributes.forward = { 0.0f, 0.0f, 1.0f };
            attributes.up = { 0.0f, 1.0f, 0.0f };
            it->second->set3DAttributes(&attributes);

            if (channel3DInfo.count(channelId)) {
                channel3DInfo[channelId].position = position;
                channel3DInfo[channelId].velocity = velocity;
            }
        }
    }

    void AudioSystem::UpdateChannel3DAttributesByName(
        const std::string& customName,
        const Vector3& position,
        const Vector3& velocity)
    {
        auto it = activeEvents.find(customName);
        if (it != activeEvents.end() && it->second) {
            FMOD_3D_ATTRIBUTES attributes = {};
            attributes.position = VectorToFmod(position);
            attributes.velocity = VectorToFmod(velocity);
            attributes.forward = { 0.0f, 0.0f, 1.0f };
            attributes.up = { 0.0f, 1.0f, 0.0f };
            it->second->set3DAttributes(&attributes);
        }
    }

    void AudioSystem::Configure3DSettings(const Audio3DSettings& settings) {
        current3DSettings = settings;
        if (coreSystem) {
            coreSystem->set3DSettings(settings.dopplerScale, settings.distanceFactor, settings.rolloffScale);
        }
    }

    //==========================================================================
    // Volume Control
    //==========================================================================

    void AudioSystem::SetMasterVolume(float volume) {
        masterVolume = std::clamp(volume, 0.0f, 1.0f);

        FMOD::Studio::Bus* masterBus = nullptr;
        if (studioSystem && studioSystem->getBus("bus:/", &masterBus) == FMOD_OK && masterBus) {
            masterBus->setVolume(masterVolume);
        }
        std::cout << "[AudioSystem] Master volume set to: " << masterVolume << std::endl;
    }

    void AudioSystem::SetBGMVolume(float volume) {
        bgmVolume = std::clamp(volume, 0.0f, 1.0f);

        FMOD::Studio::Bus* bgmBus = nullptr;
        if (studioSystem && studioSystem->getBus("bus:/BGM", &bgmBus) == FMOD_OK && bgmBus) {
            bgmBus->setVolume(bgmVolume);
        }
        std::cout << "[AudioSystem] BGM volume set to: " << bgmVolume << std::endl;
    }

    void AudioSystem::SetSFXVolume(float volume) {
        sfxVolume = std::clamp(volume, 0.0f, 1.0f);

        FMOD::Studio::Bus* sfxBus = nullptr;
        if (studioSystem && studioSystem->getBus("bus:/SFX", &sfxBus) == FMOD_OK && sfxBus) {
            sfxBus->setVolume(sfxVolume);
        }
        std::cout << "[AudioSystem] SFX volume set to: " << sfxVolume << std::endl;
    }

    void AudioSystem::SetGroupVolume(FMOD::ChannelGroup* group, float volume) {
        if (group) {
            group->setVolume(volume);
        }
    }

    void AudioSystem::PauseGroup(FMOD::ChannelGroup* group) {
        if (group) {
            bool paused = false;
            group->getPaused(&paused);
            group->setPaused(!paused);
        }
    }

    void AudioSystem::IncrementGroupVol(const std::string& groupName) {
        FMOD::Studio::Bus* bus = nullptr;
        if (studioSystem && studioSystem->getBus(groupName.c_str(), &bus) == FMOD_OK && bus) {
            float cur = 1.0f;
            bus->getVolume(&cur);
            float newV = std::min(1.0f, cur + volChangeAmount);
            bus->setVolume(newV);
            std::cout << "[AudioSystem] Increased volume of " << groupName << " to " << newV << std::endl;
        }
    }

    void AudioSystem::DecrementGroupVol(const std::string& groupName) {
        FMOD::Studio::Bus* bus = nullptr;
        if (studioSystem && studioSystem->getBus(groupName.c_str(), &bus) == FMOD_OK && bus) {
            float cur = 1.0f;
            bus->getVolume(&cur);
            float newV = std::max(0.0f, cur - volChangeAmount);
            bus->setVolume(newV);
            std::cout << "[AudioSystem] Decreased volume of " << groupName << " to " << newV << std::endl;
        }
    }

    //==========================================================================
    // BGM Fading
    //==========================================================================

    void AudioSystem::FadeBGMIn(float duration) {
        isFadingIn = true;
        isFadingOut = false;
        fadeTimer = 0.0f;
        fadeDuration = duration;
        fadeStartVolume = 0.0f;
        fadeTargetVolume = 1.0f;
    }

    void AudioSystem::FadeBGMOut(float duration) {
        isFadingOut = true;
        isFadingIn = false;
        fadeTimer = 0.0f;
        fadeDuration = duration;
        fadeStartVolume = 1.0f;
    }

    void AudioSystem::CrossfadeBGM(const std::string& newBGM, float duration) {
        // Start new BGM
        FMOD::Studio::EventDescription* eventDesc = LoadEvent(newBGM);
        if (eventDesc) {
            FMOD::Studio::EventInstance* instance = nullptr;
            if (eventDesc->createInstance(&instance) == FMOD_OK && instance) {
                instance->setVolume(0.0f);
                instance->start();

                int newChannelId = GetNextChannelId();
                activeEventsById[newChannelId] = instance;

                if (currentBGMChannel >= 0) {
                    pendingBGMChannel = newChannelId;
                    FadeBGMOut(duration);
                }
                else {
                    currentBGMChannel = newChannelId;
                    pendingBGMChannel = -1;
                    FadeBGMIn(duration);
                }
            }
        }
    }

    //==========================================================================
    // Audio Effects
    //==========================================================================

    void AudioSystem::InitializeReverb() {
        if (coreSystem) {
            coreSystem->createDSPByType(FMOD_DSP_TYPE_SFXREVERB, &reverbDSP);
            coreSystem->createDSPByType(FMOD_DSP_TYPE_LOWPASS, &lowPassFilter);
        }
    }

    void AudioSystem::ApplyReverbToChannel(int channelId, bool enable) {
        // Note: In FMOD Studio, reverb is typically applied via snapshots or buses
        (void)channelId;
        (void)enable;
    }

    void AudioSystem::SetReverbProperties(float decayTime, float wetLevel, float dryLevel) {
        if (reverbDSP) {
            reverbDSP->setParameterFloat(FMOD_DSP_SFXREVERB_DECAYTIME, decayTime);
            reverbDSP->setParameterFloat(FMOD_DSP_SFXREVERB_WETLEVEL, wetLevel);
            reverbDSP->setParameterFloat(FMOD_DSP_SFXREVERB_DRYLEVEL, dryLevel);
        }
    }

    //==========================================================================
    // Bus Management
    //==========================================================================

    void AudioSystem::CreateBus(const std::string& busPath) {
        FMOD::Studio::Bus* bus = nullptr;
        if (studioSystem && studioSystem->getBus(busPath.c_str(), &bus) == FMOD_OK && bus) {
            activeBuses[busPath] = bus;
        }
    }

    //==========================================================================
    // Global Audio Control
    //==========================================================================

    void AudioSystem::PauseAllAudio() {
        for (auto& kv : activeEvents) {
            if (kv.second) {
                // Don't pause BGM events - they're managed by SceneInterface
                if (kv.first.find("BGM") != std::string::npos) continue;
                kv.second->setPaused(true);
            }
        }
    }

    void AudioSystem::ResumeAllAudio() {
        for (auto& kv : activeEvents) {
            if (kv.second) {
                if (kv.first.find("BGM") != std::string::npos) continue;
                kv.second->setPaused(false);
            }
        }
    }

    void AudioSystem::Reset() {
        PauseAllAudio();
        for (auto& kv : activeEvents) {
            if (kv.second) {
                kv.second->stop(FMOD_STUDIO_STOP_IMMEDIATE);
            }
        }
        activeEvents.clear();
        activeEventsById.clear();
        std::cout << "[AudioSystem] Reset complete" << std::endl;
    }

    void AudioSystem::ListAllEvents() {
        std::cout << "\n[AudioSystem] === Available Events ===" << std::endl;

        int bankCapacity = 0;
        if (studioSystem->getBankList(nullptr, 0, &bankCapacity) != FMOD_OK || bankCapacity == 0) {
            std::cout << "[AudioSystem] No banks found." << std::endl;
            return;
        }

        std::vector<FMOD::Studio::Bank*> banks(bankCapacity);
        studioSystem->getBankList(banks.data(), bankCapacity, nullptr);

        for (auto* bank : banks) {
            if (!bank) continue;

            char bankPath[256];
            int pathLen = 0;
            bank->getPath(bankPath, sizeof(bankPath), &pathLen);
            std::cout << "[AudioSystem] Bank: " << bankPath << std::endl;

            int eventCount = 0;
            bank->getEventCount(&eventCount);

            if (eventCount > 0) {
                std::vector<FMOD::Studio::EventDescription*> eventList(eventCount);
                bank->getEventList(eventList.data(), eventCount, nullptr);

                for (auto* eventDesc : eventList) {
                    if (!eventDesc) continue;

                    char eventPath[256];
                    int pathSize = 0;
                    eventDesc->getPath(eventPath, sizeof(eventPath), &pathSize);
                    std::cout << "[AudioSystem]   - " << eventPath << std::endl;
                }
            }
        }
        std::cout << "[AudioSystem] ========================\n" << std::endl;
    }

    bool AudioSystem::IsEvent3D(const std::string& eventName) {
        FMOD::Studio::EventDescription* eventDesc = LoadEvent(eventName);
        if (eventDesc) {
            bool is3D = false;
            eventDesc->is3D(&is3D);
            return is3D;
        }
        return false;
    }

    //==========================================================================
    // ECS Entity Audio Control
    //==========================================================================

    void AudioSystem::PlayEntitySound(Entity entity) {
        if (!ecsInterface.HasComponent<AudioComponent>(entity)) {
            std::cerr << "[AudioSystem] Entity " << entity << " missing AudioComponent!" << std::endl;
            return;
        }

        auto& audioComp = ecsInterface.GetComponent<AudioComponent>(entity);

        // Stop existing instance
        if (audioComp.eventInstance && !audioComp.loop)
        {
            audioComp.eventInstance->stop(FMOD_STUDIO_STOP_IMMEDIATE);
            audioComp.eventInstance->release();
            audioComp.eventInstance = nullptr;
        }

        // Load event
        FMOD::Studio::EventDescription* eventDesc = LoadEvent(audioComp.eventName);
        if (!eventDesc) {
            std::cerr << "[AudioSystem] Failed to load event: " << audioComp.eventName << std::endl;
            return;
        }

        // Create instance
        FMOD_RESULT result = eventDesc->createInstance(&audioComp.eventInstance);
        if (result != FMOD_OK || !audioComp.eventInstance) {
            std::cerr << "[AudioSystem] Failed to create instance for: " << audioComp.eventName << std::endl;
            return;
        }

        // Set properties
        audioComp.eventInstance->setVolume(audioComp.volume);

        // Set 3D attributes
        if (audioComp.is3D && ecsInterface.HasComponent<TransformComponent>(entity)) {
            auto& transform = ecsInterface.GetComponent<TransformComponent>(entity);
            Update3DAttributes(entity, transform.position, glm::vec3(0.0f));
            audioComp.lastPosition = transform.position;

            audioComp.eventInstance->setProperty(FMOD_STUDIO_EVENT_PROPERTY_MINIMUM_DISTANCE, audioComp.minDistance);
            audioComp.eventInstance->setProperty(FMOD_STUDIO_EVENT_PROPERTY_MAXIMUM_DISTANCE, audioComp.maxDistance);
        }

        // Start playback
        // Start playback
        result = audioComp.eventInstance->start();
        audioComp.isPlaying = true;
        activeInstances[entity] = audioComp.eventInstance;

        std::cout << "[AudioSystem] Playing '" << audioComp.eventName << "' for entity " << entity << std::endl;

        // DEBUG: Check playback state
        FMOD_STUDIO_PLAYBACK_STATE state;
        audioComp.eventInstance->getPlaybackState(&state);
        std::cout << "[AudioSystem] Playback state: " << state
            << " (0=Playing, 1=Sustaining, 2=Stopped, 3=StartingDelayed, 4=Stopping)" << std::endl;

        // DEBUG: Check if 3D
        bool is3D = false;
        eventDesc->is3D(&is3D);
        std::cout << "[AudioSystem] Event is 3D: " << (is3D ? "YES" : "NO") << std::endl;
    }

    void AudioSystem::StopEntitySound(Entity entity) {
        if (!ecsInterface.HasComponent<AudioComponent>(entity)) return;

        auto& audioComp = ecsInterface.GetComponent<AudioComponent>(entity);

        if (audioComp.eventInstance) {
            audioComp.eventInstance->stop(FMOD_STUDIO_STOP_ALLOWFADEOUT);
            audioComp.isPlaying = false;
            activeInstances.erase(entity);
        }
    }

    void AudioSystem::SetEntityVolume(Entity entity, float volume) {
        if (!ecsInterface.HasComponent<AudioComponent>(entity)) return;

        auto& audioComp = ecsInterface.GetComponent<AudioComponent>(entity);
        audioComp.volume = glm::clamp(volume, 0.0f, 1.0f);

        if (audioComp.eventInstance) {
            audioComp.eventInstance->setVolume(audioComp.volume);
        }
    }

    void AudioSystem::SetListenerPosition(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up) {
        listenerPosition = position;
        listenerForward = glm::normalize(forward);
        listenerUp = glm::normalize(up);
    }

    //==========================================================================
    // Message Handlers
    //==========================================================================

    void AudioSystem::OnEntityCreated(Entity entity) {
        if (ecsInterface.HasComponent<AudioComponent>(entity)) {
            auto& audioComp = ecsInterface.GetComponent<AudioComponent>(entity);
            if (audioComp.playOnStart && !audioComp.eventName.empty()) {
                PlayEntitySound(entity);
            }
        }
    }

    void AudioSystem::OnEntityDestroyed(const EntityDestroyedMessage& msg) {
        auto it = activeInstances.find(msg.entity);
        if (it != activeInstances.end()) {
            if (it->second) {
                it->second->stop(FMOD_STUDIO_STOP_IMMEDIATE);
                it->second->release();
            }
            activeInstances.erase(it);
        }
    }

    void AudioSystem::OnPlaySound(const PlaySoundMessage& msg) {
        std::cout << "[AudioSystem] OnPlaySound: " << msg.soundId << std::endl;

        FMOD::Studio::EventDescription* eventDesc = LoadEvent(msg.soundId);
        if (!eventDesc) {
            std::cerr << "[AudioSystem] Failed to load event: " << msg.soundId << std::endl;
            return;
        }

        bool eventIs3D = false;
        eventDesc->is3D(&eventIs3D);

        FMOD::Studio::EventInstance* instance = nullptr;
        if (eventDesc->createInstance(&instance) != FMOD_OK || !instance) {
            std::cerr << "[AudioSystem] Failed to create instance for: " << msg.soundId << std::endl;
            return;
        }

        instance->setVolume(msg.volume);

        if (msg.is3D && eventIs3D) {
            FMOD_3D_ATTRIBUTES attributes = {};
            attributes.position = { msg.position.x, msg.position.y, msg.position.z };
            attributes.velocity = { 0.0f, 0.0f, 0.0f };
            attributes.forward = { 0.0f, 0.0f, 1.0f };
            attributes.up = { 0.0f, 1.0f, 0.0f };
            instance->set3DAttributes(&attributes);
        }

        instance->start();
    }

    void AudioSystem::OnEnterPlayMode() {
        std::cout << "[AudioSystem] Entering play mode - starting auto-play sounds..." << std::endl;

        for (Entity entity : mEntities) {
            if (!ecsInterface.HasComponent<AudioComponent>(entity)) continue;

            auto& audioComp = ecsInterface.GetComponent<AudioComponent>(entity);
            if (audioComp.playOnStart && !audioComp.eventName.empty()) {
                std::cout << "[AudioSystem] Auto-playing: " << audioComp.eventName << " for entity " << entity << std::endl;
                PlayEntitySound(entity);
            }
        }
    }

    void AudioSystem::OnExitPlayMode() {
        std::cout << "[AudioSystem] Exiting play mode - stopping all sounds..." << std::endl;

        for (Entity entity : mEntities) {
            if (!ecsInterface.HasComponent<AudioComponent>(entity)) continue;

            auto& audioComp = ecsInterface.GetComponent<AudioComponent>(entity);
            if (audioComp.isPlaying) {
                StopEntitySound(entity);
            }
        }
    }

    void AudioSystem::OnPausedPlayMode()
    {
        std::cout << "[AudioSystem] Pausing all audio..." << std::endl;
        for (auto& kv : activeEvents) {
            if (kv.second) {
                kv.second->setPaused(true);
            }
        }
    }

    void AudioSystem::OnResumePlayMode()
    {
        std::cout << "[AudioSystem] Resuming all audio..." << std::endl;
        for (auto& kv : activeEvents) {
            if (kv.second) {
                kv.second->setPaused(false);
            }
        }
    }

    //==========================================================================
    // Helper Functions
    //==========================================================================

    void AudioSystem::Update3DAttributes(Entity entity, const glm::vec3& position, const glm::vec3& velocity) {
        if (!ecsInterface.HasComponent<AudioComponent>(entity)) return;

        auto& audioComp = ecsInterface.GetComponent<AudioComponent>(entity);

        if (audioComp.eventInstance) {
            FMOD_3D_ATTRIBUTES attributes = {};
            attributes.position = { position.x, position.y, position.z };
            attributes.velocity = { velocity.x, velocity.y, velocity.z };
            attributes.forward = { 0.0f, 0.0f, 1.0f };
            attributes.up = { 0.0f, 1.0f, 0.0f };
            audioComp.eventInstance->set3DAttributes(&attributes);
        }
    }

    float AudioSystem::CalculateVolumeAttenuation(float distance, float minDist, float maxDist) {
        if (distance <= minDist) return 1.0f;
        if (distance >= maxDist) return 0.0f;
        return 1.0f - ((distance - minDist) / (maxDist - minDist));
    }

    void AudioSystem::UnloadBank(const std::string& bankPath)
    {
        auto it = loadedBanks.find(bankPath);
        if (it != loadedBanks.end())
        {
            it->second->unload();
            it->second->unloadSampleData();
            std::cout << "[AudioSystem] Bank unloaded: " << bankPath << std::endl;
            loadedBanks.erase(it);
        }
    }

    void AudioSystem::setBusMute(const std::string& path, bool mute)
    {
        FMOD::Studio::Bus* bus = nullptr;
        studioSystem->getBus(path.c_str(), &bus);
        if (bus) bus->setMute(mute);
    }

} // namespace Framework