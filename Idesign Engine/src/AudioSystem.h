///////////////////////////////////////////////////////////////////////////////
///
/// @file AudioSystem.h
/// @brief Unified audio system combining FMOD management and ECS integration
/// 
/// @Authors: Edwin Leow, Belinda Chua Jia Xuan 
/// @Copyright 2025, Digipen Institute of Technology
///
///////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef AUDIO_SYSTEM_H
#define AUDIO_SYSTEM_H

#include "pch.h"
#include "System.h"
#include "AudioComponent.h"
#include "MessageSystem.h"
#include "fmod.hpp"
#include "fmod_studio.hpp"
#include "fmod.h"
#include <glm.hpp>
#include <unordered_map>
#include <chrono>
#include <cmath>

namespace Framework {

    // Forward declarations
    struct TransformComponent;
    using Entity = uint32_t;

    //==========================================================================
    // 3D Audio Structures (from Audio.h)
    //==========================================================================

    // 3D Vector structure for spatial positioning 
    struct Vector3 {
        float x, y, z;
        Vector3(float x = 0, float y = 0, float z = 0) : x(x), y(y), z(z) {}
    };

    // Audio source types for different 3D behaviors
    enum class AudioSourceType {
        STEREO_2D,      // UI Sounds, BGM - no 3D processing
        WORLD_3D,       // Environmental sounds with full 3D
        AMBIENT_3D,     // Ambient sounds with subtle 3D
        HYBRID          // Can switch between 2D/3D
    };

    // 3D audio settings structure 
    struct Audio3DSettings {
        float dopplerScale = 1.0f;   // Doppler effect strength 
        float distanceFactor = 1.0f; // Units to meters conversion
        float rolloffScale = 1.0f;   // Distance attenuation curve 
        float minDistance = 1.0f;    // Full volume distance 
        float maxDistance = 100.0f;  // Inaudible distance 
    };

    // Channel info for tracking 3D properties 
    struct Channel3DInfo {
        Vector3 position;
        Vector3 velocity;
        float minDistance = 1.0f;
        float maxDistance = 100.0f;
        AudioSourceType type = AudioSourceType::STEREO_2D;
        bool isLooping = false;
        float baseVolume = 1.0f;
    };

    //==========================================================================
    // AudioSystem Class - Unified Audio Management
    //==========================================================================

    class AudioSystem : public ISystem {
    public:
        // Sound type enum
        enum SoundType {
            EMPTY,
            BACKGROUND_MUSIC,
            SOUND_EFFECT
        };

        AudioSystem();
        ~AudioSystem();

        //======================================================================
        // ISystem Interface
        //======================================================================
        void Initialize() override;
        void Update(float deltaTime) override;
        std::string GetName() override { return "AudioSystem"; }

        //======================================================================
        // FMOD System Management (from Audio class)
        //======================================================================

        // Core FMOD functions
        void InitializeFMOD();
        void ShutdownFMOD();
        void PrintAudioInfo() const;
        FMOD::Studio::Bank* LoadBank(const std::string& bankPath);

        // Get FMOD systems
        FMOD::Studio::System* GetStudioSystem() { return studioSystem; }
        FMOD::System* GetCoreSystem() { return coreSystem; }

        //======================================================================
        // Event Loading and Playback (from Audio class)
        //======================================================================

        FMOD_MODE GetModeFromString(const std::string& mode);
        SoundType ParseSoundType(const std::string& str);

        // Event management
        FMOD::Studio::EventDescription* LoadEvent(const std::string& eventPath);
        void PlayAudio(const std::string& eventPath);
        void PauseSound(const std::string& customName);
        void SetVolume(const std::string& customName, float volume);
        void StopChannel(int channelId);
        bool IsPlaying(const std::string& eventPath) const;
        bool IsPaused(const std::string& eventPath) const;
        FMOD::Studio::EventInstance* GetEventInstance(const std::string& eventPath) const;

        //======================================================================
        // 3D Audio Functions (from Audio class)
        //======================================================================

        int PlaySound3D(
            const std::string& customName,
            const Vector3& position,
            const Vector3& velocity = Vector3(0, 0, 0),
            float volume = 1.0f,
            AudioSourceType sourceType = AudioSourceType::WORLD_3D
        );

        void Set3DListener(
            const Vector3& position,
            const Vector3& velocity,
            const Vector3& forward,
            const Vector3& up
        );

        void Set3DListenerMultiple(
            int listenerId,
            const Vector3& position,
            const Vector3& velocity,
            const Vector3& forward,
            const Vector3& up
        );

        void UpdateChannel3DAttributes(
            int channelId,
            const Vector3& position,
            const Vector3& velocity
        );

        void UpdateChannel3DAttributesByName(
            const std::string& customName,
            const Vector3& position,
            const Vector3& velocity
        );

        void Configure3DSettings(const Audio3DSettings& settings);

        //======================================================================
        // Volume Control (from Audio class)
        //======================================================================

        void SetMasterVolume(float volume);
        void SetBGMVolume(float volume);
        void SetSFXVolume(float volume);

        float GetMasterVolume() const { return masterVolume; }
        float GetBGMVolume() const { return bgmVolume; }
        float GetSFXVolume() const { return sfxVolume; }

        void SetGroupVolume(FMOD::ChannelGroup* group, float volume);
        void PauseGroup(FMOD::ChannelGroup* group);
        void IncrementGroupVol(const std::string& groupName);
        void DecrementGroupVol(const std::string& groupName);

        void MuteMasterVolume(bool mute) { setBusMute("bus:/", mute); }
        void MuteBgmVolume(bool mute) { setBusMute("bus:/BGM", mute); }
        void MuteSfxVolume(bool mute) { setBusMute("bus:/SFX", mute); }

        //======================================================================
        // BGM Fading (from Audio class)
        //======================================================================

        void FadeBGMIn(float duration = 2.0f);
        void FadeBGMOut(float duration = 2.0f);
        void CrossfadeBGM(const std::string& newBGM, float duration = 1.0f);

        //======================================================================
        // Audio Effects (from Audio class)
        //======================================================================

        void InitializeReverb();
        void ApplyReverbToChannel(int channelId, bool enable);
        void SetReverbProperties(float decayTime, float wetLevel, float dryLevel);

        //======================================================================
        // Bus Management (from Audio class)
        //======================================================================

        void CreateBus(const std::string& busPath);

        //======================================================================
        // Global Audio Control (from Audio class)
        //======================================================================

        void PauseAllAudio();
        void ResumeAllAudio();
        void Reset();
        void ListAllEvents();

        // Check if event is 3D
        bool IsEvent3D(const std::string& eventName);

        //======================================================================
        // ECS Entity Audio Control (from AudioSystem)
        //======================================================================

        void PlayEntitySound(Entity entity);
        void StopEntitySound(Entity entity);
        void SetEntityVolume(Entity entity, float volume);
        void SetListenerPosition(const glm::vec3& position, const glm::vec3& forward, const glm::vec3& up);

        void StopEvent(const std::string& eventName);
        //======================================================================
        // Message Handlers
        //======================================================================

        void OnEntityCreated(Entity entity);
        void OnEntityDestroyed(const EntityDestroyedMessage& msg);
        void OnPlaySound(const PlaySoundMessage& msg);
        void OnEnterPlayMode();
        void OnExitPlayMode();
        void OnPausedPlayMode();
        void OnResumePlayMode();

    private:
        //======================================================================
        // Helper Functions
        //======================================================================

        // Update 3D attributes for a specific entity's sound
        void Update3DAttributes(Entity entity, const glm::vec3& position, const glm::vec3& velocity);

        // Calculate distance-based volume attenuation
        float CalculateVolumeAttenuation(float distance, float minDist, float maxDist);

        // Vector conversion helpers
        FMOD_VECTOR VectorToFmod(const Vector3& v) {
            FMOD_VECTOR fv;
            fv.x = v.x;
            fv.y = v.y;
            fv.z = v.z;
            return fv;
        }

        Vector3 FmodToVector(const FMOD_VECTOR& v) {
            return Vector3(v.x, v.y, v.z);
        }

        int GetNextChannelId() { return nextChannelId++; }

        void UnloadBank(const std::string& bankPath);

        void setBusMute(const std::string& path, bool mute);

        //======================================================================
        // FMOD System Members (from Audio class)
        //======================================================================

        FMOD::Studio::System* studioSystem = nullptr;
        FMOD::System* coreSystem = nullptr;

        // Event descriptions and instances
        std::unordered_map<std::string, FMOD::Studio::EventDescription*> loadedEvents;
        std::unordered_map<std::string, FMOD::Studio::EventInstance*> activeEvents;
        std::unordered_map<int, FMOD::Studio::EventInstance*> activeEventsById;
        std::unordered_map<int, Channel3DInfo> channel3DInfo;

        // Buses
        std::unordered_map<std::string, FMOD::Studio::Bus*> activeBuses;

        // Audio zones
        struct AudioZone {
            Vector3 minBounds;
            Vector3 maxBounds;
            float reverbAmount;
            float lowPassFilter;
        };
        std::unordered_map<std::string, AudioZone> audioZones;
        std::string currentZone;

        // 3D Settings
        Audio3DSettings current3DSettings;

        // DSP Effects
        FMOD::DSP* reverbDSP = nullptr;
        FMOD::DSP* lowPassFilter = nullptr;

        // Volume Settings
        float masterVolume = 1.0f;
        float bgmVolume = 1.0f;
        float sfxVolume = 1.0f;

        // Fade state tracking
        bool  isFadingIn = false;
        bool  isFadingOut = false;
        float fadeTimer = 0.0f;
        float fadeDuration = 2.0f;
        float fadeStartVolume = 0.0f;
        float fadeTargetVolume = 1.0f;
        int   currentBGMChannel = -1;
        int   pendingBGMChannel = -1;

        //check if audio paused 
		bool audioPaused = false;

        // Channel ID counter
        int nextChannelId = 1;

        // Misc
        const float volChangeAmount = 0.1f;
        std::chrono::steady_clock::time_point lastVolumeChangeTime;
        const std::chrono::milliseconds volumeDebounceTime{ 100 };

        // Cache of loaded banks
        std::unordered_map<std::string, FMOD::Studio::Bank*> loadedBanks;

        //======================================================================
        // ECS Members (from AudioSystem)
        //======================================================================

        // Listener properties (typically the player/camera)
        glm::vec3 listenerPosition;
        glm::vec3 listenerForward;
        glm::vec3 listenerUp;
        glm::vec3 listenerVelocity;
        glm::vec3 lastListenerPosition;

        // Cache of active audio instances for entities
        std::unordered_map<Entity, FMOD::Studio::EventInstance*> activeInstances;


    };

    // Global AudioSystem pointer (replaces GlobalAudio)
    extern AudioSystem* g_AudioSystem;

} // namespace Framework

#endif // AUDIO_SYSTEM_H