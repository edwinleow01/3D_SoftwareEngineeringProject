///////////////////////////////////////////////////////////////////////////////
///
/// @file SceneManager.cpp
/// 
/// @brief Implementation of the SceneManager system. Responsible for managing
///        scene lifecycle (loading, saving, switching, unloading) and 
///        tracking entities that belong to each scene. Implemented with intention of player being system-handled and not scene specific.
///  
/// @authors Joshua
/// @copyright 2025
///
///////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "SceneManager.h"
#include "Coordinator.h"
#include <iostream>
#include "AssetCompiler.h"
#include "AudioSystem.h"

// extern ECS interface
extern Framework::Coordinator ecsInterface;

namespace Framework {

    SceneManager::SceneManager() {}
    SceneManager::~SceneManager() {}

    SceneManager sceneManager{};

    void SceneManager::Initialize() {
        std::cout << "SceneManager initialized\n";
    }

    void SceneManager::Update(float deltaTime) {

        deltaTime = 0;
        // First handle deferred entity deletes/unloads (from previous frame)

        for (Entity e : sceneManager.m_PendingEntityDeletes) {
            ecsInterface.DestroyEntity(e);
        }
        sceneManager.m_PendingEntityDeletes.clear();

        for (const std::string& name : sceneManager.m_PendingSceneUnloads) {
            sceneManager.m_SceneEntities.erase(name);
        }
        sceneManager.m_PendingSceneUnloads.clear();


        // load any pending scene loads
        for (const auto& load : sceneManager.m_PendingSceneLoads) {
            if (!load.additive) {
                ecsInterface.ClearEntities();
                sceneManager.m_SceneEntities.clear();
                sceneManager.m_CurrentSceneName.clear();
            }

            sceneManager.m_SceneEntities[load.sceneName].clear();
            loadScene(load.fullPath);

            sceneManager.m_CurrentSceneName = load.sceneName;

            // Set the BGM path for this scene - USE sceneManager. prefix!
            auto it = sceneManager.m_SceneBGM.find(load.sceneName);
            if (it != sceneManager.m_SceneBGM.end() && !it->second.empty()) {
                sceneManager.m_CurrentSceneBGMPath = it->second;
                std::cout << "[SceneManager] Scene BGM set to: " << it->second << std::endl;
            }
            else {
                sceneManager.m_CurrentSceneBGMPath.clear();
                std::cout << "[SceneManager] No BGM mapped for scene: " << load.sceneName << std::endl;
            }

            // ACTUALLY PLAY THE BGM
            if (g_AudioSystem && !sceneManager.m_CurrentSceneBGMPath.empty()) {
                // Stop any currently playing BGM first
                g_AudioSystem->StopEvent(sceneManager.m_CurrentSceneBGMPath);

                // Play the new BGM
                g_AudioSystem->PlayAudio(sceneManager.m_CurrentSceneBGMPath);
                std::cout << "[SceneManager] Playing BGM: " << sceneManager.m_CurrentSceneBGMPath << std::endl;
            }
        }
        sceneManager.m_PendingSceneLoads.clear();
    }



    std::string SceneManager::GetName() {
        return "SceneManager";
    }

    Entity SceneManager::SpawnPrefab(const std::string& prefabGUID)
    {
        Entity entity = InstantiatePrefab(prefabGUID);

        if (entity != std::numeric_limits<Entity>::max())
        {
            // Register to current scene
            if (!m_CurrentSceneName.empty())
                RegisterEntityToScene(entity, m_CurrentSceneName);
        }

        return entity;
    }

    Entity SceneManager::SpawnPrefab(const std::string& prefabGUID, const glm::vec3& position)
    {
        Entity entity = InstantiatePrefab(prefabGUID, position);

        if (entity != std::numeric_limits<Entity>::max())
        {
            if (!m_CurrentSceneName.empty())
                RegisterEntityToScene(entity, m_CurrentSceneName);
        }

        return entity;
    }

    Entity SceneManager::SpawnPrefab(const std::string& prefabGUID, const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale)
    {
        Entity entity = InstantiatePrefab(prefabGUID, position, rotation, scale);

        if (entity != std::numeric_limits<Entity>::max())
        {
            if (!m_CurrentSceneName.empty())
                RegisterEntityToScene(entity, m_CurrentSceneName);
        }

        return entity;
    }


    // Load scene from file, additive to not clear entities before loading, else it will do a full clear of entities
    void SceneManager::LoadScene(const std::string& path, bool additive) {
        // Extract clean scene name from path (strip directories and extension)
        std::string cleanName = path;
        size_t lastSlash = cleanName.find_last_of("/\\");
        if (lastSlash != std::string::npos)
            cleanName = cleanName.substr(lastSlash + 1);
        size_t dot = cleanName.find_last_of('.');
        if (dot != std::string::npos)
            cleanName = cleanName.substr(0, dot);

        // Queue both path and clean name
        m_PendingSceneLoads.push_back({ path, cleanName, additive });

    }

    void SceneManager::SaveScene(const std::string& path) {
        saveScene(path);
    }

    // Call to unload only specifc scene
    void SceneManager::UnloadScene(const std::string& name) {
        // Clean the input name in case caller passed a path
        std::string cleanName = name;
        size_t lastSlash = cleanName.find_last_of("/\\");
        if (lastSlash != std::string::npos)
            cleanName = cleanName.substr(lastSlash + 1);
        size_t dot = cleanName.find_last_of('.');
        if (dot != std::string::npos)
            cleanName = cleanName.substr(0, dot);

        if (m_SceneEntities.find(cleanName) == m_SceneEntities.end()) return;

        // Defer entity deletes
        for (Entity e : m_SceneEntities[cleanName]) {
            m_PendingEntityDeletes.push_back(e);
        }

        m_PendingSceneUnloads.push_back(cleanName);

        if (m_CurrentSceneName == cleanName)
            m_CurrentSceneName.clear();

    }


    // ------------------  Helper functions -----------------

    std::string SceneManager::GetCurrentSceneName() const {
        return m_CurrentSceneName;
    }

    void SceneManager::RegisterEntityToScene(Entity entity, const std::string& sceneName) {
        m_SceneEntities[sceneName].push_back(entity);
    }
}
