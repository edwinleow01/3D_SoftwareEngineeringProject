///////////////////////////////////////////////////////////////////////////////
///
///	@File  : AssetManager.h
/// @Brief : This file implements the AssetManager class, a central component 
///          responsible for managing various asset types within the application, 
///          including audio, textures, fonts, and shaders. The AssetManager 
///          provides methods to load and retrieve these assets, allowing for 
///          efficient reuse across different systems.
///
///	@Authors : Edwin
///	@Copyright 2024, Digipen Institute of Technology
///
///////////////////////////////////////////////////////////////////////////////
#pragma once
#ifndef _ASSET_MANAGER_H_
#define _ASSET_MANAGER_H_
#include "pch.h"

// Standard Lib files
#include <unordered_map>
#include <optional>
#include <functional>
#include "EnginePath.h" // <-- Added this
#include <filesystem>   // <-- Added this

// Graphics Header Files
#include "stb_image.h"
#include "stb_image_resize2.h"

// Properties Files
#include "PropertySystem.h"
#include "WindowConfigProperties.h"
#include "TextureProperties.h"
#include "MeshProperties.h"
#include "TextProperties.h"
#include "NavMeshAgentProperties.h"
#include "NavMeshProperties.h"
#include "PrefabProperties.h" 
#include "SceneProperties.h"
#include "MaterialProperties.h"
#include "AnimationProperties.h"
#include "LightProperties.h"
#include "PhysicsProperties.h"
#include "AudioComponent.h"
#include "ScriptProperties.h"
#include "EventReceiverComponent.h"
#include "EventSenderComponent.h"
#include "BoxColliderComponent.h"
#include "ButtonComponent.h"
#include "TimerComponent.h"

// Asset Files
#include "AssetLoader.h"
#include "AssetHandler.h"
#include "MaterialLoader.h"


namespace Framework
{
    // This registering affects the following functions below
    // SaveArrayToJson etc... can add more once found, hopefully change during holiday
    REGISTER_TYPE_NAME(WindowConfig);
    REGISTER_TYPE_NAME(AudioComponent);
    REGISTER_TYPE_NAME(ListenerComponent);
    REGISTER_TYPE_NAME(AudioBankMetadata);
    REGISTER_TYPE_NAME(TextureProperties);
    REGISTER_TYPE_NAME(MeshProperties);
    REGISTER_TYPE_NAME(TextProperties);
    REGISTER_TYPE_NAME(NavMeshComponent);
    REGISTER_TYPE_NAME(NavMeshAgentComponent);
    REGISTER_TYPE_NAME(PrefabProperties);
    REGISTER_TYPE_NAME(SceneProperties);
    REGISTER_TYPE_NAME(MaterialProperties);
    REGISTER_TYPE_NAME(MaterialMetadata);
    REGISTER_TYPE_NAME(AnimationProperties);
    REGISTER_TYPE_NAME(LightProperties);
    REGISTER_TYPE_NAME(PhysicsProperties);
    REGISTER_TYPE_NAME(ScriptProperties);
    REGISTER_TYPE_NAME(ScriptEntry);
    REGISTER_TYPE_NAME(EventReceiverComponent);
    REGISTER_TYPE_NAME(EventSenderComponent);
    REGISTER_TYPE_NAME(BoxColliderComponent);
    REGISTER_TYPE_NAME(ButtonComponent);
    REGISTER_TYPE_NAME(TimerComponent);

    struct AssetKey 
    {
        AssetHandle guid;
        std::string typeName;

        bool operator==(const AssetKey& other) const 
        {
            return guid == other.guid && typeName == other.typeName;
        }
    };

    struct AssetKeyHash 
    {
        std::size_t operator()(const AssetKey& k) const noexcept 
        {
            std::size_t h1 = std::hash<AssetHandle>()(k.guid);
            std::size_t h2 = std::hash<std::string>()(k.typeName);
            return h1 ^ (h2 + 0x9e3779b97f4a7c15ULL + (h1 << 6) + (h1 >> 2));
        }
    };

    /*  Struct Brief
        - Store any type of assets in a generic way
        - Track metadata for serialization and debugging
        - Allow safetly movable but not copyable 
    */
    struct AssetEntry
    {
        std::unique_ptr<void, void(*)(void*)> pObject;  // Type-erased smart pointer to hold any asset object with a custom deleter
        const prop_list* pProps;                        // Pointer to the property list used for serialization/deserialization
        std::string typeName;                           // String to store the type name of the asset
        std::string binaryPath;                         // Asset file path
        AssetHandle guid;                               // Unique identifier for the asset

        AssetEntry() : pObject(nullptr, [](void*) {}), pProps(nullptr), typeName(), binaryPath(), guid() {}   // Default Constructor   
        AssetEntry(const AssetEntry&) = delete;                                         // Delete Copy Constructor
        AssetEntry& operator=(const AssetEntry&) = delete;                              // Delete Copy Assignment
        AssetEntry(AssetEntry&&) = default;                                             // Move Constructor
        AssetEntry& operator=(AssetEntry&&) = default;                                  // Move Assignment
        AssetEntry(std::unique_ptr<void, void(*)(void*)> obj,                           // Constructor 
                   const prop_list* props, 
                   const std::string& type, 
                   const std::string& path, 
                   AssetHandle guid)
                   : pObject(std::move(obj)), pProps(props), typeName(type), binaryPath(path), guid(guid) {}
    };

    class AssetManager
    {
    public:
        static AssetManager& Instance()
        {
            static AssetManager instance;
            return instance;
        }

        // Delete copy/move constructors to enforce singleton
        AssetManager(const AssetManager&) = delete;
        AssetManager& operator=(const AssetManager&) = delete;
        ~AssetManager()
        {
            m_Assets.clear();
        }

        //------------------------------------------------------------------------------------------------------------------------------------
        //  Binary Related Functions
        //  Json > Binary > Read Binary during Runtime
        //------------------------------------------------------------------------------------------------------------------------------------
        template<typename T>
        T& LoadBinary(const std::string& binaryPath, const prop_list& props, std::function<void(T&)> postInit = nullptr)
        {
            auto obj = AssetLoader<T>::Load(binaryPath, props);
            AssetHandle guidHandle(obj->guid);

            if (postInit) postInit(*obj);

            // Register using composite key
            RegisterAsset(std::move(obj), &props, binaryPath);

            // Build composite key for lookup
            AssetKey key{ guidHandle, TypeName<T>::value };

            return *static_cast<T*>(m_Assets[key].pObject.get());
        }

        template<typename T>
        void LoadBinaryArray(const std::string& binaryPath, const prop_list& props, std::function<void(T&)> postInit = nullptr)
        {
            auto loadedObjects = AssetLoader<T>::LoadArray(binaryPath, props);
            for (auto& obj : loadedObjects)
            {
                if (postInit) postInit(*obj); 

                RegisterAsset(std::move(obj), &props, binaryPath); // Move after init
            }
        }

        template<typename T>
        void LoadBinaryArrayFromFolder(const std::string& folderPath, const prop_list& props, std::function<void(T&)> postInit = nullptr)
        {
            for (const auto& entry : std::filesystem::directory_iterator(folderPath))
            {
                if (!entry.is_regular_file()) continue;

                const auto& path = entry.path();
                if (path.extension() == ".bin") LoadBinaryArray<T>(path.string(), props, postInit);
            }
        }

        //------------------------------------------------------------------------------------------------------------------------------------
        //  General Functions
        //------------------------------------------------------------------------------------------------------------------------------------        
        template<typename T>
        T* Get(const AssetHandle& guid)
        {
            AssetKey key{ guid, TypeName<T>::value };
            auto it = m_Assets.find(key);
            if (it == m_Assets.end()) return nullptr;
            return static_cast<T*>(it->second.pObject.get());
        }

        template<typename T>
        void RegisterAsset(std::unique_ptr<T> obj, const prop_list* props, const std::string& binaryPath)
        {
            if (obj->guid.empty()) obj->guid = GenerateGUID();
            AssetHandle guidHandle(obj->guid);

            std::unique_ptr<void, void(*)(void*)> erasedPtr(
                static_cast<void*>(obj.release()),
                [](void* ptr) { delete static_cast<T*>(ptr); }
            );

            AssetKey key{ guidHandle, TypeName<T>::value };
            m_Assets[key] = AssetEntry(std::move(erasedPtr), props, TypeName<T>::value, binaryPath, guidHandle);
        }

        // Template function to find the GUID of an asset of type T.
        // Optionally filters by name if provided.
        template<typename T>
        AssetHandle FindGuid(const std::optional<std::string>& name = std::nullopt)
        {
            // [CRITICAL RECOMMENDATION] 
            // If your game is multi-threaded, you MUST lock the asset manager here.
            // If AssetManager::Instance().GetAssets() is modified while this loop runs, 
            // the app WILL crash randomly.
            // std::lock_guard<std::mutex> lock(AssetManager::Instance().GetMutex()); 

            const auto& assets = AssetManager::Instance().GetAssets();

            for (const auto& [key, entry] : assets)
            {
                // 1. Safety: Check if the raw pointer is null immediately
                if (!entry.pObject)
                    continue;

                // 2. Type Check
                if (entry.typeName != TypeName<T>::value)
                    continue;

                // 3. Cast
                T* obj = static_cast<T*>(entry.pObject.get());

                // 4. Safety: Double check the cast result
                if (!obj)
                    continue;

                // 5. Comparison Logic
                if constexpr (requires { std::declval<T>().name; })
                {
                    if (name.has_value())
                    {
                        // FIX: Removed std::string_view wrappers. 
                        // Creating a string_view from a garbage pointer causes an immediate crash.
                        // Standard operator== is safer and handles std::string vs char* correctly.
                        if (obj->name == name.value())
                        {
                            return key.guid;
                        }
                    }
                    else
                    {
                        // Name required but not provided
                        return key.guid;
                    }
                }
                else if (!name.has_value())
                {
                    return key.guid;
                }
            }

            return AssetHandle();
        }

        // Access to Unordered Map of Assets
        const std::unordered_map<AssetKey, AssetEntry, AssetKeyHash>& GetAssets() const
        {
            return m_Assets;
        }

        void SaveAllAssets();
        void CompileAllAssets();
        void LoadAllAssets();
        bool DeleteAssetFile(const std::string& filePath);
        void Unload(const std::string& guid);
        bool SaveToJson(void* pObject, const std::string& filePath, const prop_list& props);
        void SaveArrayToJson(const std::string& typeName, const std::string& filePath, const std::string& arrayKey);
        bool HasTextureByGUID(const std::string& guid) const;

    private:
        AssetManager() = default;                                   // Private constructor
        std::unordered_map<AssetKey, AssetEntry, AssetKeyHash> m_Assets;
    };
}
#endif // !_ASSET_MANAGER_H_