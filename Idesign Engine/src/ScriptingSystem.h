#pragma once
#include "System.h"
#include <string>
#include <vector>
#include <iostream>
#include <lua.hpp>       // Required for lua_State in templates
#include "Coordinator.h" // Required for ecsInterface in templates
#include "ScriptProperties.h"

// Forward Declarations
namespace Framework { class Graphics; }
extern Framework::Coordinator ecsInterface;

namespace Framework {

    class ScriptingSystem : public ISystem {
    public:
        ScriptingSystem();
        ~ScriptingSystem() override;

        void Initialize() override;
        void Update(float dt) override;
        std::string GetName() override;

        // --- Setup & Config ---
        void SetGraphicsSystem(Graphics* g) { graphics_ = g; }
        Graphics* GetGraphicsSystem() const { return graphics_; }

        static void SetProjectRoot(const std::string& root);
        static std::string GetProjectRoot();

        Entity CreateScriptEntity(const std::string& path);
        void HotReloadScript(const std::string& changedPath);
        lua_State* GetLuaState() const { return L; }

        static inline bool SkipCutscene = false; // Toggle to skip cutscene before submission

        // =================================================================================
        //  THE UNIVERSAL CALL FUNCTION (Template Implementation)
        //  Must be in the header file!
        // =================================================================================

        // Usage: 
        // Call(e, "OnEvent");              -> Void (Broadcast to all scripts)
        // Call<bool>(e, "OnPickup");       -> Bool (Stops on first script that returns true)
        template <typename T = void, typename... Args>
        T Call(Entity entity, const std::string& funcName, Args... args)
        {
            // 1. Safety Checks
            if (!ecsInterface.IsEntityValid(entity) || !ecsInterface.HasComponent<Framework::ScriptProperties>(entity)) {
                return GetDefault<T>();
            }

            auto& comp = ecsInterface.GetComponent<Framework::ScriptProperties>(entity);

            // 2. Loop through scripts
            for (auto& sc : comp.scripts) {
                // Check validity (handles -1 default and -2 NOREF)
                if (sc.tableRef == LUA_NOREF || sc.tableRef == -1) continue;

                // A. Get Table
                lua_rawgeti(L, LUA_REGISTRYINDEX, sc.tableRef);
                if (!lua_istable(L, -1)) { lua_pop(L, 1); continue; }

                // B. Get Function
                lua_getfield(L, -1, funcName.c_str());
                if (!lua_isfunction(L, -1)) { lua_pop(L, 2); continue; }

                // C. Push Arguments
                // We assume the first arg is ALWAYS the Entity ID for consistency
                PushToLua(static_cast<int>(entity));
                (PushToLua(args), ...); // Fold expression

                int argCount = 1 + sizeof...(Args);
                int resultCount = std::is_same<T, void>::value ? 0 : 1;

                // D. Call Lua
                if (lua_pcall(L, argCount, resultCount, 0) == LUA_OK)
                {
                    // E. Handle Return Value
                    if constexpr (!std::is_same<T, void>::value) {
                        T result = GetFromLua<T>(-1);
                        lua_pop(L, 1); // Pop result
                        lua_pop(L, 1); // Pop table
                        return result; // RETURN IMMEDIATELY (Override Mode)
                    }
                    else {
                        lua_pop(L, 1); // Pop table
                        // CONTINUE LOOPING (Event Broadcast Mode)
                    }
                }
                else {
                    std::cerr << "[Lua Error] " << lua_tostring(L, -1) << std::endl;
                    lua_pop(L, 1); // Pop error
                    lua_pop(L, 1); // Pop table
                }
            }
            return GetDefault<T>();
        }
        std::unordered_map<std::string, std::vector<std::pair<int, int>>> m_LuaEventRegistry;
    private:
        lua_State* L = nullptr;
        Graphics* graphics_ = nullptr;

        // Static member declaration
        static inline std::string s_ProjectRoot;

        // Internal Helpers
        bool LoadScriptTable(ScriptEntry& inst);
        bool CallHook(int tableRef, const char* fn, float arg = 0.0f);

    private:
        // 1. Standard Int
        void PushToLua(int v) { lua_pushinteger(L, v); }

        // 2. Unsigned Int (Fixes 'Entity' ambiguity)
        void PushToLua(unsigned int v) { lua_pushinteger(L, static_cast<lua_Integer>(v)); }

        // 3. Long Long (Handles large numbers/size_t)
        void PushToLua(long long v) { lua_pushinteger(L, v); }
        void PushToLua(unsigned long long v) { lua_pushinteger(L, static_cast<lua_Integer>(v)); }

        // 4. Floats
        void PushToLua(float v) { lua_pushnumber(L, v); }
        void PushToLua(double v) { lua_pushnumber(L, v); }

        // 5. Bool
        void PushToLua(bool v) { lua_pushboolean(L, v); }

        // 6. Strings
        void PushToLua(const std::string& v) { lua_pushstring(L, v.c_str()); }
        void PushToLua(const char* v) { lua_pushstring(L, v); }

        // --- GET HELPERS ---
        template <typename T> T GetFromLua(int index);
        template <typename T> T GetDefault() { return T(); }
    };

    // --- TEMPLATE SPECIALIZATIONS (Implementation) ---
    template<> inline bool ScriptingSystem::GetFromLua<bool>(int index) { return lua_toboolean(L, index); }
    template<> inline int ScriptingSystem::GetFromLua<int>(int index) { return (int)lua_tointeger(L, index); }
    template<> inline float ScriptingSystem::GetFromLua<float>(int index) { return (float)lua_tonumber(L, index); }
    template<> inline std::string ScriptingSystem::GetFromLua<std::string>(int index) { return lua_tostring(L, index); }
    template<> inline void ScriptingSystem::GetFromLua<void>(int index) { return; }

} // namespace Framew