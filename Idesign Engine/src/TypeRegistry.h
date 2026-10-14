#pragma once
#ifndef _TYPE_REGISTRY_H_
#define _TYPE_REGISTRY_H_

#include <string>
#include <unordered_map>
#include <functional>
#include "Coordinator.h"

namespace Framework
{
    using TypeGetter = std::function<void* (Coordinator&, Entity)>;

    class TypeRegistry
    {
    private:
  
        // Private constructor ensures it's only created via Instance()
        TypeRegistry() = default;

    public:
        // Delete copy/move
        TypeRegistry(const TypeRegistry&) = delete;
        TypeRegistry& operator=(const TypeRegistry&) = delete;

        static TypeRegistry& Instance()
        {
            if (!s_Instance) {
                s_Instance = new TypeRegistry();
            }
            return *s_Instance;
        }

        // [CHANGE 2] Static Shutdown to delete the instance
        static void Shutdown()
        {
            if (s_Instance)
            {
                s_Instance->ClearData(); // Clear internal maps
                delete s_Instance;       // Delete the object itself
                s_Instance = nullptr;    // Reset pointer
            }
        }

        // Internal clear function (was your old Shutdown)
        void ClearData()
        {
            std::unordered_map<std::string, TypeGetter> emptyRegistry;
            registry.swap(emptyRegistry);

            std::unordered_map<std::string, TypeAdder> emptyAdders;
            adders.swap(emptyAdders);

            std::unordered_map<std::string, TypeRemover> emptyRemovers;
            removers.swap(emptyRemovers);

            propLists.clear();
        }

        using TypeAdder = std::function<void(Coordinator&, Entity)>;
        using TypeRemover = std::function<void(Coordinator&, Entity)>;

        template<typename T>
        void Register(const std::string& typeName, const prop_list* props)
        {
            registry[typeName] = [](Coordinator& ecs, Entity e) -> void*
                {
                    if (ecs.HasComponent<T>(e))
                        return &ecs.GetComponent<T>(e);
                    return nullptr;
                };

            adders[typeName] = [](Coordinator& ecs, Entity e)
                {
                    ecs.AddComponent<T>(e, T{});
                };

            removers[typeName] = [](Coordinator& ecs, Entity e)
                {
                    ecs.RemoveComponent<T>(e);
                };

            propLists[typeName] = props;
        }

        inline const prop_list* GetPropList(const std::string& typeName) const
        {
            auto it = propLists.find(typeName);
            return (it != propLists.end()) ? it->second : nullptr;
        }

        inline bool RemoveComponent(const std::string& typeName, Coordinator& ecs, Entity e)
        {
            auto* remover = GetRemover(typeName);
            if (!remover) return false;
            (*remover)(ecs, e);
            return true;
        }

        inline bool AddComponent(const std::string& typeName, Coordinator& ecs, Entity e)
        {
            auto* adder = GetAdder(typeName);
            if (!adder) return false;
            (*adder)(ecs, e);
            return true;
        }

        inline TypeAdder* GetAdder(const std::string& typeName)
        {
            auto it = adders.find(typeName);
            return (it != adders.end()) ? &it->second : nullptr;
        }

        inline TypeRemover* GetRemover(const std::string& typeName)
        {
            auto it = removers.find(typeName);
            return (it != removers.end()) ? &it->second : nullptr;
        }

        TypeGetter* Get(const std::string& typeName)
        {
            auto it = registry.find(typeName);
            return (it != registry.end()) ? &it->second : nullptr;
        }

        const std::unordered_map<std::string, TypeGetter>& GetAll() const { return registry; }

    private:
        inline static TypeRegistry* s_Instance = nullptr;
        std::unordered_map<std::string, TypeGetter> registry;
        std::unordered_map<std::string, TypeAdder> adders;
        std::unordered_map<std::string, TypeRemover> removers;
        std::unordered_map<std::string, const prop_list*> propLists;
    };
}
#endif // !_TYPE_REGISTRY_H_