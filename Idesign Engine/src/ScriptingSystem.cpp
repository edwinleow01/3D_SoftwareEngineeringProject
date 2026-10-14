#include "pch.h"
#include "ScriptingSystem.h"
#include "Coordinator.h"
#include "Graphics.h"          // to call Graphics* methods
#include <lua.hpp>
#include "PlayerInteractSystem.h"
#include <iostream>
#include "AssetCompiler.h"
#include "WindowConfigProperties.h"  // g_WindowConfigProps
#include "TextureLoader.h"           // g_TexturePropertiesProps
#include "NavMeshSystem.h"
#include "MeshProperties.h"          // g_MeshPropertiesProps
#include <filesystem>
#include "ScriptProperties.h"
#include "AudioSystem.h"
#include "ItemComponent.h"
#include <GameSystem.h>
#include <PhysicsSystem.h>
#include "EngineState.h"
#include "ButtonComponent.h"
#include "ButtonSystem.h"
#include "SceneManager.h"
#include "EventManager.h"
#include "Inventory.h"
#include "BoxColliderComponent.h"
#include "InputHandler.h"
#include "SceneInterface.h"
#include "DialogueSystem.h"
#include "Inventory.h"
#include "PhaseManager.h"
#include <Core.h>
#include "Upgrade.h"
#include <ParticleComponent.h>

extern Framework::Coordinator ecsInterface;

namespace Framework {

    // -------- file-scope helpers (must NOT be inside a member function) --------
    namespace {
        // Where we store/retrieve the ScriptingSystem* inside Lua's registry
        constexpr const char* kRegistryKey = "__ScriptingSystemPtr";

        inline ScriptingSystem* lua_self(lua_State* L)
        {
            lua_getfield(L, LUA_REGISTRYINDEX, kRegistryKey);
            auto* self = static_cast<ScriptingSystem*>(lua_touserdata(L, -1));
            lua_pop(L, 1);
            return self;
        }

        static int lua_GetPosition(lua_State* L)
        {
            // 1st arg: entity id
            Entity e = static_cast<Entity>(luaL_checkinteger(L, 1));

            if (!ecsInterface.IsEntityValid(e) || !ecsInterface.HasComponent<TransformComponent>(e))
            {
                // no transform; return 0 values
                return luaL_error(L, "GetPosition: entity %d has no TransformComponent", (int)e);
            }

            auto& tr = ecsInterface.GetComponent<TransformComponent>(e);

            lua_pushnumber(L, tr.position.x);
            lua_pushnumber(L, tr.position.y);
            lua_pushnumber(L, tr.position.z);
            return 3; // x, y, z
        }

        static int l_StopEntityAnimation(lua_State* L) {
            // 1. Get Entity ID
            Entity entity = static_cast<Entity>(luaL_checkinteger(L, 1));

            // 2. Check for Component
            if (ecsInterface.IsEntityValid(entity) && ecsInterface.HasComponent<AnimationProperties>(entity)) {
                auto& animProps = ecsInterface.GetComponent<AnimationProperties>(entity);

                // 3. Set speed to 0 to freeze the animation
                animProps.animationSpeed = 0.0f;
            }
            return 0;
        }
        static int lua_SetPosition(lua_State* L)
        {
            Entity e = static_cast<Entity>(luaL_checkinteger(L, 1));
            float x = static_cast<float>(luaL_checknumber(L, 2));
            float y = static_cast<float>(luaL_checknumber(L, 3));
            float z = static_cast<float>(luaL_checknumber(L, 4));

            if (!ecsInterface.IsEntityValid(e) || !ecsInterface.HasComponent<TransformComponent>(e))
            {
                return luaL_error(L, "SetPosition: entity %d has no TransformComponent", (int)e);
            }
            // --- ADD THIS PRINT STATEMENT ---
            if (ecsInterface.GetEntityName(e) == "Player") {
                std::cout << "[Camera Debug] Moving Player to: X=" << x << " Y=" << y << " Z=" << z << std::endl;
            }
            // --------------------------------
                       auto& tr = ecsInterface.GetComponent<TransformComponent>(e);
            tr.position = glm::vec3(x, y, z);
            return 0;
        }

        static int lua_GetScale(lua_State* L)
        {
            // 1st arg: entity id
            Entity e = static_cast<Entity>(luaL_checkinteger(L, 1));

            if (!ecsInterface.IsEntityValid(e) || !ecsInterface.HasComponent<TransformComponent>(e))
            {
                return luaL_error(L, "GetScale: entity %d has no TransformComponent", (int)e);
            }

            auto& tr = ecsInterface.GetComponent<TransformComponent>(e);

            lua_pushnumber(L, tr.scale.x);
            lua_pushnumber(L, tr.scale.y);
            lua_pushnumber(L, tr.scale.z);
            return 3; // x, y, z
        }

        static int lua_SetIsUI(lua_State* L)
        {
            int entityId = (int)luaL_checkinteger(L, 1);
            bool value = lua_toboolean(L, 2);

            Entity e = (Entity)entityId;
            if (ecsInterface.IsEntityValid(e) && ecsInterface.HasComponent<TextureProperties>(e))
            {
                ecsInterface.GetComponent<TextureProperties>(e).isUI = value;
            }
            return 0;
        }

        int l_IsKeyDown(lua_State* L)
        {
            auto* self = lua_self(L);
            Graphics* g = self ? self->GetGraphicsSystem() : nullptr;     // <-- use getter
            if (!g) { lua_pushboolean(L, 0); return 1; }
            GLFWwindow* win = g->GetWindow();
            int key = (int)luaL_checkinteger(L, 1);
            int down = (win && glfwGetKey(win, key) == GLFW_PRESS) ? 1 : 0;
            lua_pushboolean(L, down);
            return 1;
        }

        int l_CameraMove(lua_State* L)
        {
            auto* self = lua_self(L);
            Graphics* g = self ? self->GetGraphicsSystem() : nullptr;     // <-- use getter
            if (!g) return 0;
            const char* dir = luaL_checkstring(L, 1);
            float dt = (float)luaL_checknumber(L, 2);
            bool gm = (lua_gettop(L) >= 3) ? (lua_toboolean(L, 3) != 0) : g->GetGroundModeFromLua();
            g->MoveCameraFromLua(dir, dt, gm);
            return 0;
        }

        static int lua_GetMousePosition(lua_State* L)
        {
            // Access your singleton InputHandler
            auto* input = Framework::InputHandler::GetInstance();

            if (input)
            {
                // Push X and Y to Lua
                lua_pushnumber(L, input->GetMouseX());
                lua_pushnumber(L, input->GetMouseY());
                return 2; // We are returning 2 values (x, y)
            }

            // Fallback if system is missing
            lua_pushnumber(L, 0.0);
            lua_pushnumber(L, 0.0);
            return 2;
        }

        static int lua_GetMouseUV(lua_State* L)
        {
            auto* input = Framework::InputHandler::GetInstance();
            auto* self = lua_self(L);
            Graphics* g = self ? self->GetGraphicsSystem() : nullptr;

            if (input && g && g->GetWindow())
            {
                int width, height;
                glfwGetWindowSize(g->GetWindow(), &width, &height);

                // Prevent division by zero if window is minimized
                if (width == 0 || height == 0) {
                    lua_pushnumber(L, 0.0);
                    lua_pushnumber(L, 0.0);
                    return 2;
                }

                float uvX = static_cast<float>(input->GetMouseX()) / static_cast<float>(width);
                float uvY = static_cast<float>(input->GetMouseY()) / static_cast<float>(height);

                lua_pushnumber(L, uvX);
                lua_pushnumber(L, uvY);
                return 2;
            }

            lua_pushnumber(L, 0.0);
            lua_pushnumber(L, 0.0);
            return 2;
        }

        //For text component
        static int lua_GetTextComponent(lua_State* L)
        {
            Entity e = static_cast<Entity>(luaL_checkinteger(L, 1));
            if (!ecsInterface.IsEntityValid(e) || !ecsInterface.HasComponent<TextProperties>(e)) {
                lua_pushnil(L);
                return 1;
            }
            auto& text = ecsInterface.GetComponent<TextProperties>(e);
            // Return a table with text component data
            lua_newtable(L);
            lua_pushstring(L, text.text.c_str());
            lua_setfield(L, -2, "text");
            lua_pushnumber(L, text.scale);
            lua_setfield(L, -2, "scale");

            // Color
            lua_newtable(L);
            lua_pushnumber(L, text.color.r); lua_setfield(L, -2, "r");
            lua_pushnumber(L, text.color.g); lua_setfield(L, -2, "g");
            lua_pushnumber(L, text.color.b); lua_setfield(L, -2, "b");
            lua_setfield(L, -2, "color");

            // Opacity (NEW)
            lua_pushnumber(L, text.opacity);
            lua_setfield(L, -2, "opacity");

            lua_pushinteger(L, text.alignment);
            lua_setfield(L, -2, "alignment");
            lua_pushboolean(L, text.isBillboard);
            lua_setfield(L, -2, "isBillboard");
            lua_pushboolean(L, text.screenSpace);
            lua_setfield(L, -2, "screenSpace");
            return 1;
        }

        //text component
        static int lua_SetTextComponent(lua_State* L)
        {
            Entity e = static_cast<Entity>(luaL_checkinteger(L, 1));
            const char* field = luaL_checkstring(L, 2);
            // Value is at index 3 (type depends on field)
            if (!ecsInterface.IsEntityValid(e) || !ecsInterface.HasComponent<TextProperties>(e)) {
                return luaL_error(L, "SetTextComponent: invalid entity or missing TextProperties");
            }
            auto& text = ecsInterface.GetComponent<TextProperties>(e);

            if (strcmp(field, "text") == 0) {
                text.text = luaL_checkstring(L, 3);
            }
            else if (strcmp(field, "scale") == 0) {
                text.scale = static_cast<float>(luaL_checknumber(L, 3));
            }
            else if (strcmp(field, "opacity") == 0) {  // NEW
                text.opacity = static_cast<float>(luaL_checknumber(L, 3));
            }
            else if (strcmp(field, "alignment") == 0) {
                text.alignment = static_cast<int>(luaL_checkinteger(L, 3));
            }
            else if (strcmp(field, "isBillboard") == 0) {
                text.isBillboard = lua_toboolean(L, 3);
            }
            else if (strcmp(field, "screenSpace") == 0) {
                text.screenSpace = lua_toboolean(L, 3);
            }
            else if (strcmp(field, "color") == 0) {
                // Expect a table with r, g, b
                if (!lua_istable(L, 3)) {
                    return luaL_error(L, "SetTextComponent: color must be a table {r, g, b}");
                }
                lua_getfield(L, 3, "r");
                lua_getfield(L, 3, "g");
                lua_getfield(L, 3, "b");
                text.color.r = static_cast<float>(luaL_checknumber(L, -3));
                text.color.g = static_cast<float>(luaL_checknumber(L, -2));
                text.color.b = static_cast<float>(luaL_checknumber(L, -1));
                lua_pop(L, 3);
            }
            else {
                return luaL_error(L, "SetTextComponent: unknown field '%s'", field);
            }
            return 0;
        }

        // Transform component
        static int lua_SetTransformComponent(lua_State* L)
        {
            Entity e = static_cast<Entity>(luaL_checkinteger(L, 1));
            const char* field = luaL_checkstring(L, 2);
            // Value is at index 3 (type depends on field)

            if (!ecsInterface.IsEntityValid(e) || !ecsInterface.HasComponent<TransformComponent>(e)) {
                return luaL_error(L, "SetTransformComponent: invalid entity or missing TransformComponent");
            }

            auto& transform = ecsInterface.GetComponent<TransformComponent>(e);

            if (strcmp(field, "position") == 0) {
                // Expect a table with x, y, z
                if (!lua_istable(L, 3)) {
                    return luaL_error(L, "SetTransformComponent: position must be a table {x, y, z}");
                }
                lua_getfield(L, 3, "x");
                lua_getfield(L, 3, "y");
                lua_getfield(L, 3, "z");
                transform.position.x = static_cast<float>(luaL_checknumber(L, -3));
                transform.position.y = static_cast<float>(luaL_checknumber(L, -2));
                transform.position.z = static_cast<float>(luaL_checknumber(L, -1));
                lua_pop(L, 3);
            }
            else if (strcmp(field, "rotation") == 0) {
                // Expect a table with x, y, z
                if (!lua_istable(L, 3)) {
                    return luaL_error(L, "SetTransformComponent: rotation must be a table {x, y, z}");
                }
                lua_getfield(L, 3, "x");
                lua_getfield(L, 3, "y");
                lua_getfield(L, 3, "z");
                transform.rotation.x = static_cast<float>(luaL_checknumber(L, -3));
                transform.rotation.y = static_cast<float>(luaL_checknumber(L, -2));
                transform.rotation.z = static_cast<float>(luaL_checknumber(L, -1));
                lua_pop(L, 3);
            }
            else if (strcmp(field, "scale") == 0) {
                // Expect a table with x, y, z
                if (!lua_istable(L, 3)) {
                    return luaL_error(L, "SetTransformComponent: scale must be a table {x, y, z}");
                }
                lua_getfield(L, 3, "x");
                lua_getfield(L, 3, "y");
                lua_getfield(L, 3, "z");
                transform.scale.x = static_cast<float>(luaL_checknumber(L, -3));
                transform.scale.y = static_cast<float>(luaL_checknumber(L, -2));
                transform.scale.z = static_cast<float>(luaL_checknumber(L, -1));
                lua_pop(L, 3);
            }
            else if (strcmp(field, "isActive") == 0) {
                transform.isActive = lua_toboolean(L, 3);
            }
            else {
                return luaL_error(L, "SetTransformComponent: unknown field '%s'", field);
            }

            return 0;
        }

        static int l_spawnCauldronItem(lua_State* L) {
            // 1. Get Arguments
            Entity parentID = static_cast<Entity>(luaL_checkinteger(L, 1));
            int itemID = (int)luaL_checkinteger(L, 2);

            // 2. Validate Parent
            if (!ecsInterface.IsEntityValid(parentID) ||
                !ecsInterface.HasComponent<TransformComponent>(parentID)) {
                return 0;
            }

            // 3. Validate Item ID
            if (itemID < 0 || itemID >= Framework::kItemCount) {
                std::cout << "[Lua Error] Invalid Item ID: " << itemID << "\n";
                return 0;
            }

            std::string itemName = Framework::kItemNames[itemID];

            // =========================================================
            // PATH HANDLING LOGIC (Robust)
            // =========================================================

            // 1. Define Relative Paths
            std::string relObjPath = "Assets/Objects/" + itemName + ".obj";
            std::string relPngPath = "Assets/Textures/" + itemName + "3D.png";

            // 2. Resolve Mesh Path
            fs::path meshPath(relObjPath);
            if (!meshPath.is_absolute()) {
                // If relative, prepend Current Working Directory (Project Root)
                meshPath = fs::current_path() / meshPath;
            }

            // 3. Resolve Texture Path
            fs::path texPath(relPngPath);
            if (!texPath.is_absolute()) {
                texPath = fs::current_path() / texPath;
            }

            // 4. Debug: Verify existence (Helps you see if path is wrong)
            if (!fs::exists(meshPath)) {
                std::cout << "[Lua Error] MESH MISSING at: " << meshPath.string() << std::endl;
                // Optional: Fallback to a default cube if missing
                // meshPath = fs::current_path() / "Assets/Objects/cube.obj";
            }
            if (!fs::exists(texPath)) {
                std::cout << "[Lua Error] TEXTURE MISSING at: " << texPath.string() << std::endl;
            }

            // =========================================================

            // 5. Calculate Position (Above BoxCollider)
            auto& parentTrans = ecsInterface.GetComponent<TransformComponent>(parentID);
            glm::vec3 spawnPos = parentTrans.position;

            // 6. Create Entity
            Entity itemEntity = ecsInterface.CreateEntity();

            // --- NEW: SET UNIQUE NAME ---
            std::string uniqueName = itemName + "_" + std::to_string((unsigned int)itemEntity);

            // Assuming you have a SetEntityName function in your ECS Interface
            ecsInterface.SetEntityName(itemEntity, uniqueName);
            std::cout << "[DEBUG C++] Created Entity: " << uniqueName << std::endl;
            // ----------------------------

            // 7. Transform
            TransformComponent tf;
            tf.position = spawnPos;
            tf.scale = glm::vec3(0.3f);
            tf.rotation = glm::vec3(-90.0f, 0.0f, 0.0f);
            ecsInterface.AddComponent(itemEntity, tf);

            // 8. Hierarchy
            HierarchyComponent hc;
            ecsInterface.AddComponent(itemEntity, hc);

            // 9. Mesh (Use the resolved absolute path)
            MeshProperties mp;
            mp.path = meshPath.string(); // Convert fs::path back to string
            MeshLoader::loadMesh(mp.path, mp);
            ecsInterface.AddComponent(itemEntity, mp);

            // 10. Texture (Use the resolved absolute path)
            TextureProperties tp;
            tp.path = texPath.string(); // Convert fs::path back to string
            TextureLoader::loadTexture(tp, true);
            ecsInterface.AddComponent(itemEntity, tp);

            ItemComponent itemComp;
            itemComp.name = itemName;
            itemComp.isIngredient = true;
            itemComp.isChoppable = false;
            itemComp.toDestroyWhenPicked = true; // Crucial: C++ destroys it on pickup
            ecsInterface.AddComponent(itemEntity, itemComp);

            TagComponent tc;
            tc.tags = { "Interactable" };
            ecsInterface.AddComponent(itemEntity, tc);

            PhysicsProperties pp;
            pp.isImmovable = true;
            pp.category = 1;
            ecsInterface.AddComponent(itemEntity, pp);

            BoxColliderComponent colliderComp;
            BoxColliderEntry entry;
            entry.size = glm::vec3(1.0f);
            entry.isInteractable = true;
            entry.promptText = "Pick up: " + itemName;
            colliderComp.colliders.push_back(entry);
            ecsInterface.AddComponent(itemEntity, colliderComp);

            lua_pushinteger(L, static_cast<lua_Integer>(itemEntity));
            return 1;
        }

        static int l_isItemZipping(lua_State* L) {
            Entity ent = static_cast<Entity>(luaL_checkinteger(L, 1));
            if (ecsInterface.IsEntityValid(ent) && ecsInterface.HasComponent<ItemComponent>(ent)) {
                auto& item = ecsInterface.GetComponent<ItemComponent>(ent);
                lua_pushboolean(L, item.isZipping);
                return 1;
            }
            lua_pushboolean(L, false);
            return 1;
        }

        static int l_SetInteractionText(lua_State* L)
        {
            // Arg 1: Entity ID
            Entity entity = static_cast<Entity>(luaL_checkinteger(L, 1));

            // Arg 2: New Text
            const char* newText = luaL_checkstring(L, 2);

            // Logic: Find the collider and update the string
            if (ecsInterface.IsEntityValid(entity))
            {
                if (ecsInterface.HasComponent<BoxColliderComponent>(entity))
                {
                    auto& colliderComp = ecsInterface.GetComponent<BoxColliderComponent>(entity);

                    // Loop through all colliders attached to this entity
                    for (auto& entry : colliderComp.colliders)
                    {
                        // We only want to update the ones marked as Interactable
                        if (entry.isInteractable)
                        {
                            entry.promptText = std::string(newText);
                        }
                    }
                }
            }

            return 0; // Returns nothing to Lua
        }

        int Lua_SetColliderPromptText(lua_State* L) {
            int entityId = (int)lua_tointeger(L, 1);
            const char* text = lua_tostring(L, 2);
            Entity e = (Entity)entityId;
            if (ecsInterface.IsEntityValid(e) && ecsInterface.HasComponent<BoxColliderComponent>(e)) {
                auto& col = ecsInterface.GetComponent<BoxColliderComponent>(e);
                if (!col.colliders.empty()) {
                    col.colliders[0].promptText = text;
                }
            }
            return 0;
        }

        int Lua_SetColliderInteractable(lua_State* L) {
            int entityId = (int)lua_tointeger(L, 1);
            bool state = lua_toboolean(L, 2);
            Entity e = (Entity)entityId;
            if (ecsInterface.IsEntityValid(e) && ecsInterface.HasComponent<BoxColliderComponent>(e)) {
                auto& col = ecsInterface.GetComponent<BoxColliderComponent>(e);
                if (!col.colliders.empty())
                    col.colliders[0].isInteractable = state;
            }
            return 0;
        }

        static int l_DestroyEntity(lua_State* L) {
            // 1. Get Entity ID from Lua
            Entity e = static_cast<Entity>(luaL_checkinteger(L, 1));

            // 2. Check validity
            if (ecsInterface.IsEntityValid(e)) {

                sceneManager.m_PendingEntityDeletes.push_back(e);
                // std::cout << "[Lua] Queued Entity " << e << " for destruction" << std::endl;
            }
            else {
                std::cout << "[Lua Warning] Tried to destroy invalid entity: " << e << std::endl;
            }
            return 0;
        }

        //Event firing
        static int lua_FireEvent(lua_State* L)
        {
            // Arg 1: Event Message Name (string)
            const char* messageName = luaL_checkstring(L, 1);

            // Arg 2: Sender Entity (optional, defaults to 0)
            Entity senderID = 0;
            if (lua_gettop(L) >= 2) {
                senderID = static_cast<Entity>(luaL_checkinteger(L, 2));
            }

            // Arg 3: Target Entity (optional, defaults to 0)
            Entity targetID = 0;
            if (lua_gettop(L) >= 3) {
                targetID = static_cast<Entity>(luaL_checkinteger(L, 3));
            }

            // Arg 4: Action Type (optional, defaults to 0)
            int actionType = 0;
            if (lua_gettop(L) >= 4) {
                actionType = static_cast<int>(luaL_checkinteger(L, 4));
            }

            // Build Payload
            CollisionEventData payload;
            payload.senderID = senderID;
            payload.targetID = targetID;
            payload.actionType = actionType;

            // Fire Event
            EventManager::Instance().Fire(messageName, &payload);

            std::cout << "[Lua] Fired Event: " << messageName << std::endl;

            return 0;
        }

        static int lua_SubscribeToEvent(lua_State* L)
        {
            // Arg 1: Event Name (string) - e.g., "RequestHerbSpawn"
            const char* messageName = luaL_checkstring(L, 1);

            // Arg 2: Lua Function Name (string) - e.g., "RespawnHerbs"
            const char* funcNameStr = luaL_checkstring(L, 2);

            // Arg 3: Entity ID (integer) - The entity that owns the script
            Entity ownerEntity = static_cast<Entity>(luaL_checkinteger(L, 3));

            // Get ScriptingSystem pointer from registry
            auto* self = lua_self(L);
            if (!self) return 0;

            // Use a string copy for the lambda capture
            std::string luaFuncName = funcNameStr;

            // Register with C++ EventManager
            EventManager::Instance().Subscribe(messageName, [self, ownerEntity, luaFuncName](void* payload) {

                // 1. Validation: If entity is gone, unsubscribe
                if (!ecsInterface.IsEntityValid(ownerEntity)) {
                    return false;
                }

                return true; // Stay subscribed
                });

            std::cout << "[Lua] Entity " << ownerEntity << " Subscribed to: " << messageName << std::endl;
            return 0;
        }

        // Set a script's variable (Access a scripts local var and edit if needed)
        static int lua_SetScriptVariable(lua_State* L)
        {
            Entity e = static_cast<Entity>(luaL_checkinteger(L, 1));
            const char* varName = luaL_checkstring(L, 2);
            const char* value = luaL_checkstring(L, 3);

            if (!ecsInterface.IsEntityValid(e) || !ecsInterface.HasComponent<ScriptProperties>(e)) {
                return luaL_error(L, "SetScriptVariable: invalid entity or no ScriptProperties");
            }

            auto& scriptProps = ecsInterface.GetComponent<ScriptProperties>(e);

            // Find the Customer script's table
            for (auto& sc : scriptProps.scripts) {
                if (sc.tableRef != LUA_NOREF && sc.tableRef > 0) {
                    // Push the script table
                    lua_rawgeti(L, LUA_REGISTRYINDEX, sc.tableRef);

                    // Set the variable
                    lua_pushstring(L, value);
                    lua_setfield(L, -2, varName);

                    // Pop the table
                    lua_pop(L, 1);
                }
            }

            return 0;
        }

        // Engine LUA exposed utility functions (Get entity, get by component, get by name, tag , create prefab, setactive -deactive etc

        int lua_FindEntityByName(lua_State* L)
        {
            const char* name = luaL_checkstring(L, 1);
            Entity e = ecsInterface.GetEntityByName(name);
            if (ecsInterface.IsEntityValid(e)) {
                lua_pushinteger(L, static_cast<int>(e));
            }
            else {
                lua_pushnil(L);
            }
            return 1;
        }

        // Navmesh build function
        static int lua_BuildNavMesh(lua_State* L)
        {
            auto& ecs = ecsInterface;
            auto navSystem = ecs.GetSystem<Framework::NavMeshSystem>();

            if (!navSystem)
            {
                std::cerr << "[Lua] NavMeshSystem not found!\n";
                lua_pushboolean(L, false);
                return 1;
            }

            // If your function is void and doesn't return success:
            navSystem->BuildNavMesh();  // or whatever your method is called
            std::cout << "[Lua] NavMesh build triggered.\n";

            lua_pushboolean(L, true);
            return 1;
        }

        //Spawn prefab expose
        static int lua_SpawnPrefab(lua_State* L)
        {
            const char* prefabIdentifier = luaL_checkstring(L, 1);

            Entity spawnedEntity = std::numeric_limits<Entity>::max();

            int numArgs = lua_gettop(L);

            if (numArgs == 1)
            {
                // Try as GUID first (direct lookup - faster)
                spawnedEntity = InstantiatePrefab(std::string(prefabIdentifier));

                // If that fails, try as name
                if (spawnedEntity == std::numeric_limits<Entity>::max())
                {
                    spawnedEntity = InstantiatePrefabByName(std::string(prefabIdentifier));
                }
            }
            else if (numArgs == 2)
            {
                // Spawn at another entity's position
                Entity positionEntity = static_cast<Entity>(luaL_checkinteger(L, 2));

                if (!ecsInterface.IsEntityValid(positionEntity) ||
                    !ecsInterface.HasComponent<TransformComponent>(positionEntity))
                {
                    return luaL_error(L, "SpawnPrefab: invalid position entity or missing TransformComponent");
                }

                auto& posTransform = ecsInterface.GetComponent<TransformComponent>(positionEntity);

                // Try GUID first
                spawnedEntity = InstantiatePrefab(std::string(prefabIdentifier), posTransform.position);

                // Try name if GUID failed
                if (spawnedEntity == std::numeric_limits<Entity>::max())
                {
                    spawnedEntity = InstantiatePrefabByName(std::string(prefabIdentifier), posTransform.position);
                }
            }
            else if (numArgs == 4)
            {
                // Spawn with explicit position (x, y, z)
                float x = static_cast<float>(luaL_checknumber(L, 2));
                float y = static_cast<float>(luaL_checknumber(L, 3));
                float z = static_cast<float>(luaL_checknumber(L, 4));

                glm::vec3 position(x, y, z);

                // Try GUID first
                spawnedEntity = InstantiatePrefab(std::string(prefabIdentifier), position);

                // Try name if GUID failed
                if (spawnedEntity == std::numeric_limits<Entity>::max())
                {
                    spawnedEntity = InstantiatePrefabByName(std::string(prefabIdentifier), position);
                }
            }
            else if (numArgs == 10)
            {
                // Full transform
                float px = static_cast<float>(luaL_checknumber(L, 2));
                float py = static_cast<float>(luaL_checknumber(L, 3));
                float pz = static_cast<float>(luaL_checknumber(L, 4));

                float rx = static_cast<float>(luaL_checknumber(L, 5));
                float ry = static_cast<float>(luaL_checknumber(L, 6));
                float rz = static_cast<float>(luaL_checknumber(L, 7));

                float sx = static_cast<float>(luaL_checknumber(L, 8));
                float sy = static_cast<float>(luaL_checknumber(L, 9));
                float sz = static_cast<float>(luaL_checknumber(L, 10));

                // Try GUID first
                spawnedEntity = InstantiatePrefab(
                    std::string(prefabIdentifier),
                    glm::vec3(px, py, pz),
                    glm::vec3(rx, ry, rz),
                    glm::vec3(sx, sy, sz)
                );

                // Try name if GUID failed
                if (spawnedEntity == std::numeric_limits<Entity>::max())
                {
                    spawnedEntity = InstantiatePrefabByName(
                        std::string(prefabIdentifier),
                        glm::vec3(px, py, pz),
                        glm::vec3(rx, ry, rz),
                        glm::vec3(sx, sy, sz)
                    );
                }
            }
            else
            {
                return luaL_error(L, "SpawnPrefab: invalid number of arguments (expected 1, 2, 4, or 10)");
            }

            // Return the spawned entity ID (or nil if failed)
            if (spawnedEntity == std::numeric_limits<Entity>::max())
            {
                lua_pushnil(L);
                return 1;
            }

            lua_pushinteger(L, static_cast<lua_Integer>(spawnedEntity));
            return 1;
        }

        // Dialogue / cutscenes
        static int lua_CutsceneSetTarget(lua_State* L) {
            const char* tag = luaL_checkstring(L, 1);
            auto dialogueSys = ecsInterface.GetSystem<Framework::DialogueSystem>();
            if (dialogueSys) dialogueSys->SetCameraTargetByTag(tag);
            return 0;
        }

        static int lua_CutsceneSetOffset(lua_State* L) {
            float x = (float)luaL_checknumber(L, 1);
            float y = (float)luaL_checknumber(L, 2);
            float z = (float)luaL_checknumber(L, 3);
            auto dialogueSys = ecsInterface.GetSystem<Framework::DialogueSystem>();
            if (dialogueSys) dialogueSys->flyOffset = glm::vec3(x, y, z);
            return 0;
        }

        static int lua_CutsceneSetSpeed(lua_State* L) {
            float speed = (float)luaL_checknumber(L, 1);
            auto dialogueSys = ecsInterface.GetSystem<Framework::DialogueSystem>();
            if (dialogueSys) dialogueSys->flySpeed = speed;
            return 0;
        }

        static int lua_CutsceneStart(lua_State* L) {

            if (ScriptingSystem::SkipCutscene) {
                return 0;
            }

            auto dialogueSys = ecsInterface.GetSystem<Framework::DialogueSystem>();
            if (dialogueSys) {
                dialogueSys->setActive(!ScriptingSystem::SkipCutscene);  // <-- THIS IS THE KEY LINE= true;
                dialogueSys->isCutscene = !ScriptingSystem::SkipCutscene;
                dialogueSys->isMidFlight = !ScriptingSystem::SkipCutscene;
                engineState.PushState(EngineMode::Dialogue);  // block all input
            }
            return 0;
        }

        static int lua_CutsceneIsFlying(lua_State* L) {
            auto dialogueSys = ecsInterface.GetSystem<Framework::DialogueSystem>();
            bool flying = dialogueSys ? dialogueSys->isMidFlight : false;
            lua_pushboolean(L, flying);
            return 1;
        }

        // --------- Tag management functions--------------

        static int lua_GetEntitiesByTag(lua_State* L)
        {
            const char* tag = luaL_checkstring(L, 1);

            std::vector<Entity> entities = ecsInterface.GetEntitiesByTag(tag);

            // Create a Lua table to return the entities
            lua_newtable(L);

            for (size_t i = 0; i < entities.size(); ++i)
            {
                lua_pushinteger(L, static_cast<lua_Integer>(entities[i]));
                lua_rawseti(L, -2, static_cast<int>(i + 1)); // Lua arrays start at 1
            }

            return 1; // Return the table
        }

        static int lua_AddTagToEntity(lua_State* L)
        {
            Entity e = static_cast<Entity>(luaL_checkinteger(L, 1));
            const char* tag = luaL_checkstring(L, 2);

            if (!ecsInterface.IsEntityValid(e))
            {
                return luaL_error(L, "AddTagToEntity: invalid entity %d", (int)e);
            }

            ecsInterface.AddTagToEntity(e, tag);
            return 0;
        }

        static int lua_RemoveTagFromEntity(lua_State* L)
        {
            Entity e = static_cast<Entity>(luaL_checkinteger(L, 1));
            const char* tag = luaL_checkstring(L, 2);

            if (!ecsInterface.IsEntityValid(e))
            {
                return luaL_error(L, "RemoveTagFromEntity: invalid entity %d", (int)e);
            }

            if (!ecsInterface.HasComponent<TagComponent>(e))
            {
                return 0; // Nothing to remove
            }

            auto& tagComp = ecsInterface.GetComponent<TagComponent>(e);
            auto it = std::find(tagComp.tags.begin(), tagComp.tags.end(), std::string(tag));

            if (it != tagComp.tags.end())
            {
                tagComp.tags.erase(it);
            }

            return 0;
        }

        static int lua_EntityHasTag(lua_State* L)
        {
            Entity e = static_cast<Entity>(luaL_checkinteger(L, 1));
            const char* tag = luaL_checkstring(L, 2);

            bool hasTag = ecsInterface.EntityHasTag(e, tag);
            lua_pushboolean(L, hasTag);
            return 1;
        }

        static int lua_SetTexture(lua_State* L)
        {
            // 1. Get arguments: Entity ID and Texture Path
            Entity e = static_cast<Entity>(luaL_checkinteger(L, 1));
            const char* path = luaL_checkstring(L, 2);

            // 2. Validate Entity & Component
            if (ecsInterface.IsEntityValid(e) && ecsInterface.HasComponent<TextureProperties>(e)) {
                auto& tex = ecsInterface.GetComponent<TextureProperties>(e);

                // 3. Update Path
                tex.path = path;

                // 4. Reload Texture Data Immediately
                TextureLoader::loadTexture(tex, true);
            }
            return 0;
        }

        static int lua_SetMesh(lua_State* L)
        {
            // 1. Get arguments: Entity ID and Mesh Path
            Entity e = static_cast<Entity>(luaL_checkinteger(L, 1));
            const char* path = luaL_checkstring(L, 2);

            // 2. Validate Entity & Component
            if (ecsInterface.IsEntityValid(e) && ecsInterface.HasComponent<MeshProperties>(e)) {
                auto& mesh = ecsInterface.GetComponent<MeshProperties>(e);

                // 3. Update Path
                mesh.path = path;

                // 4. Reload Mesh Data Immediately
                MeshLoader::loadMesh(mesh.path, mesh);
            }
            return 0;
        }

        static int lua_SetRotation(lua_State* L)
        {
            Entity e = static_cast<Entity>(luaL_checkinteger(L, 1));
            float x = static_cast<float>(luaL_checknumber(L, 2));
            float y = static_cast<float>(luaL_checknumber(L, 3));
            float z = static_cast<float>(luaL_checknumber(L, 4));

            if (!ecsInterface.IsEntityValid(e) ||
                !ecsInterface.HasComponent<TransformComponent>(e))
            {
                return luaL_error(L, "SetRotation: entity %d has no TransformComponent", (int)e);
            }

            auto& tr = ecsInterface.GetComponent<TransformComponent>(e);
            tr.rotation = glm::vec3(x, y, z);
            return 0;
        }

        // Update your existing lua bindings
        static int lua_GetScore(lua_State* L)
        {
            //int s = Framework::GameSystem::GetScore();
            //lua_pushinteger(L, s);
            return 1;
        }

        static int lua_SetScore(lua_State* L)
        {
            //int value = (int)luaL_checkinteger(L, 1);
            //Framework::GameSystem::SetScore(value);
            return 0;
        }

        // Add new generic data functions
        static int lua_GetData(lua_State* L)
        {
            const char* key = luaL_checkstring(L, 1);
            int value = Framework::GameSystem::GetData(key);
            lua_pushinteger(L, value);
            return 1;
        }

        static int lua_SetData(lua_State* L)
        {
            const char* key = luaL_checkstring(L, 1);
            int value = (int)luaL_checkinteger(L, 2);
            Framework::GameSystem::SetData(key, value);
            return 0;
        }

        static int lua_AddData(lua_State* L)
        {
            const char* key = luaL_checkstring(L, 1);
            int delta = (int)luaL_checkinteger(L, 2);
            Framework::GameSystem::AddData(key, delta);
            return 0;
        }

        static int lua_HasData(lua_State* L)
        {
            const char* key = luaL_checkstring(L, 1);
            bool exists = Framework::GameSystem::HasData(key);
            lua_pushboolean(L, exists);
            return 1;
        }

        static int lua_SetText(lua_State* L)
        {
            int id = (int)luaL_checkinteger(L, 1);
            const char* newText = luaL_checkstring(L, 2);

            Entity e = static_cast<Entity>(id);

            if (ecsInterface.IsEntityValid(e) && ecsInterface.HasComponent<TextProperties>(e))
            {
                auto& txt = ecsInterface.GetComponent<TextProperties>(e);
                txt.text = newText;
            }
            return 0;
        }

        // Dialogue functions
        // 
        // // Add these inside the anonymous namespace with other Lua functions

       // In the anonymous namespace with other Lua functions:

        static int lua_DialogueClear(lua_State* L) {
            auto dialogueSys = ecsInterface.GetSystem<Framework::DialogueSystem>();
            if (dialogueSys) dialogueSys->Clear();
            return 0;
        }

        static int lua_DialogueAdd(lua_State* L) {
            const char* text = luaL_checkstring(L, 1);
            const char* speaker = luaL_optstring(L, 2, "");
            const char* portrait = luaL_optstring(L, 3, "");
            const char* voice = luaL_optstring(L, 4, "");

            auto dialogueSys = ecsInterface.GetSystem<Framework::DialogueSystem>();
            if (dialogueSys) dialogueSys->AddLine(text, speaker, portrait, voice);
            return 0;
        }

        static int lua_DialogueStart(lua_State* L) {

            if (ScriptingSystem::SkipCutscene) {
                return 0;
            }
         

            auto dialogueSys = ecsInterface.GetSystem<Framework::DialogueSystem>();
            if (dialogueSys) dialogueSys->StartDialogue();
            return 0;
        }

        static int lua_DialogueEnd(lua_State* L) {
            auto dialogueSys = ecsInterface.GetSystem<Framework::DialogueSystem>();
            if (dialogueSys) dialogueSys->EndDialogue();
            return 0;
        }

        static int lua_DialogueIsActive(lua_State* L) {
            auto dialogueSys = ecsInterface.GetSystem<Framework::DialogueSystem>();
            bool active = dialogueSys ? dialogueSys->IsActive() : false;
            lua_pushboolean(L, active);
            return 1;
        }

        // ============ AUDIO LUA BINDINGS ============
        static int lua_AudioPlay(lua_State* L)
        {
            const char* eventPath = luaL_checkstring(L, 1);
            if (g_AudioSystem) g_AudioSystem->PlayAudio(eventPath);
            return 0;
        }

        static int lua_AudioStop(lua_State* L)
        {
            if (g_AudioSystem)
            {
                // Check if an event name was provided
                if (lua_gettop(L) >= 1 && lua_isstring(L, 1))
                {
                    // Stop specific event
                    const char* eventName = lua_tostring(L, 1);
                    g_AudioSystem->StopEvent(eventName);
                }
                else g_AudioSystem->Reset(); // No arguments - stop all audio (legacy behavior)
            }
            return 0;
        }

        static int lua_AudioPause(lua_State* L)
        {
            (void)L;
            if (g_AudioSystem) g_AudioSystem->PauseAllAudio();
            return 0;
        }

        static int lua_AudioResume(lua_State* L)
        {
            (void)L;
            if (g_AudioSystem) g_AudioSystem->ResumeAllAudio();
            return 0;
        }

        static int lua_AudioSetVolume(lua_State* L)
        {
            float volume = (float)luaL_checknumber(L, 1);
            if (g_AudioSystem)
            {
                g_AudioSystem->SetMasterVolume(volume);
            }
            return 0;
        }

        static int lua_AudioPlay3D(lua_State* L)
        {
            const char* eventPath = luaL_checkstring(L, 1);
            float x = (float)luaL_checknumber(L, 2);
            float y = (float)luaL_checknumber(L, 3);
            float z = (float)luaL_checknumber(L, 4);

            if (g_AudioSystem) g_AudioSystem->PlaySound3D(eventPath, Vector3(x, y, z));
            return 0;
        }

        static int lua_AudioIsPlaying(lua_State* L)
        {
            const char* eventPath = luaL_checkstring(L, 1);
            bool isPlaying = false;

            if (g_AudioSystem) isPlaying = g_AudioSystem->IsPlaying(eventPath);

            lua_pushboolean(L, isPlaying);
            return 1;
        }

        static int lua_AudioMute(lua_State* L)
        {
            const char* tag = luaL_checkstring(L, 1); // "master", "bgm", "sfx"
            bool mute = lua_toboolean(L, 2);

            if (strcmp(tag, "master_mute") == 0) g_AudioSystem->MuteMasterVolume(mute);
            else if (strcmp(tag, "bgm_mute") == 0) g_AudioSystem->MuteBgmVolume(mute);
            else if (strcmp(tag, "sfx_mute") == 0) g_AudioSystem->MuteSfxVolume(mute);

            return 0;
        }

        // ============ END AUDIO LUA BINDINGS ============

        // ============================================================================
        // ENGINE STATE LUA BINDINGS - ADD THESE
        // ============================================================================

        // Helper to convert string to EngineMode
        static EngineMode StringToEngineMode(const std::string& stateName)
        {
            if (stateName == "Editor")          return EngineMode::Editor;
            if (stateName == "Play")            return EngineMode::Play;
            if (stateName == "Paused")          return EngineMode::Paused;
            if (stateName == "Debug")           return EngineMode::Debug;
            if (stateName == "Menu")            return EngineMode::Menu;
            if (stateName == "HowToPlay")       return EngineMode::HowToPlay;
            if (stateName == "Upgrade")         return EngineMode::Upgrade;
            if (stateName == "Settings_Audio")  return EngineMode::Settings_Audio;
            if (stateName == "Settings_Gamma")  return EngineMode::Settings_Gamma;
            if (stateName == "Dialogue")        return EngineMode::Dialogue;
            if (stateName == "MapSelection")    return EngineMode::MapSelection;
            if (stateName == "DayStats")        return EngineMode::DayStats;
            if (stateName == "EndStats")        return EngineMode::EndStats;
            if (stateName == "Intro")           return EngineMode::Intro;
            if (stateName == "Cutscene")        return EngineMode::Cutscene;
            if (stateName == "Win")             return EngineMode::Win;
            if (stateName == "Lose")            return EngineMode::Lose;
            if (stateName == "Credit")          return EngineMode::Credit;
            if (stateName == "Day2")            return EngineMode::Day2;
            if (stateName == "Day3")            return EngineMode::Day3;
            if (stateName == "Day4")            return EngineMode::Day4;
            if (stateName == "Day5")            return EngineMode::Day5;

            std::cerr << "[Lua] Unknown EngineMode: " << stateName << std::endl;
            return EngineMode::Play; // Default fallback
        }

        // Helper to convert EngineMode to string
        static std::string EngineModeToString(EngineMode mode)
        {
            switch (mode)
            {
            case EngineMode::Editor:         return "Editor";
            case EngineMode::Play:           return "Play";
            case EngineMode::Paused:         return "Paused";
            case EngineMode::Debug:          return "Debug";
            case EngineMode::Menu:           return "Menu";
            case EngineMode::HowToPlay:      return "HowToPlay";
            case EngineMode::Upgrade:        return "Upgrade";
            case EngineMode::Settings_Audio: return "Settings_Audio";
            case EngineMode::Settings_Gamma: return "Settings_Gamma";
            case EngineMode::Dialogue:       return "Dialogue";
            case EngineMode::MapSelection:   return "MapSelection";
            case EngineMode::DayStats:       return "DayStats";
            case EngineMode::EndStats:       return "EndStats";
            case EngineMode::Intro:          return "Intro";
            case EngineMode::Cutscene:       return "Cutscene";
            case EngineMode::Win:            return "Win";
            case EngineMode::Lose:           return "Lose";
            case EngineMode::Credit:         return "Credit";
            case EngineMode::Day2:           return "Day2";
            case EngineMode::Day3:           return "Day3";
            case EngineMode::Day4:           return "Day4";
            case EngineMode::Day5:           return "Day5";
            default: return "Unknown";
            }
        }

        // Lua Function: PushEngineState(stateName)
        static int lua_PushEngineState(lua_State* L)
        {
            const char* stateName = luaL_checkstring(L, 1);
            EngineMode mode = StringToEngineMode(std::string(stateName));

            engineState.PushState(mode);

            std::cout << "[Lua] Pushed EngineState: " << stateName << std::endl;
            return 0;
        }

        // Lua Function: PopEngineState()
        static int lua_PopEngineState(lua_State* L)
        {
            (void)L; // Unused parameter
            engineState.PopState();

            std::cout << "[Lua] Popped EngineState, now: "
                << EngineModeToString(engineState.Current()) << std::endl;
            return 0;
        }

        // Lua Function: ClearEngineStates()
        static int lua_ClearEngineStates(lua_State* L)
        {
            (void)L; // Unused parameter
            engineState.ClearToBase();

            std::cout << "[Lua] Cleared all states to base" << std::endl;
            return 0;
        }

        // Lua Function: GetCurrentEngineState()
        static int lua_GetCurrentEngineState(lua_State* L)
        {
            std::string current = EngineModeToString(engineState.Current());
            lua_pushstring(L, current.c_str());
            return 1;
        }

        // Lua Function: IsEngineState(stateName)
        static int lua_IsEngineState(lua_State* L)
        {
            const char* stateName = luaL_checkstring(L, 1);
            EngineMode mode = StringToEngineMode(std::string(stateName));

            bool result = engineState.Is(mode);
            lua_pushboolean(L, result);
            return 1;
        }

        // Lua Function: HasEngineState(stateName)
        static int lua_HasEngineState(lua_State* L)
        {
            const char* stateName = luaL_checkstring(L, 1);
            EngineMode mode = StringToEngineMode(std::string(stateName));

            bool result = engineState.HasState(mode);
            lua_pushboolean(L, result);
            return 1;
        }

        // ============================================================================
        // END ENGINE STATE BINDINGS
        // ============================================================================

        // ============ GAME STATE LUA BINDINGS ============
        static int lua_GameStateStart(lua_State* L)
        {
            (void)L;
            engineState.ClearToBase();                                  // Clear everything except base
            engineState.PushState(EngineMode::Play);                    // Push gameplay layer
            phaseManager->SetPhase(GamePhase::Day);                     // Set game phase
            //Framework::Inventory::Instance().TryAddItem_BackpackFirst(3, 0, 1);
            MessageBus::Get().SendImmediate(EnterPlayModeMessage());    // Send message to start audio and other play-mode systems
            //InstantiatePrefabByName("ForestHerb_Prefab");
            return 0;
        }

        static int lua_GameStateResume(lua_State* L)
        {
            (void)L;
            while (!engineState.Is(EngineMode::Play)) engineState.PopState();   // Pop temporary states until we are back to Play
            MessageBus::Get().SendImmediate(ResumePlayModeMessage());           // Send message to resume audio and gameplay systems

            return 0;
        }

        static int lua_GameStatePaused(lua_State* L)
        {
            (void)L;
            engineState.PushState(EngineMode::Paused);                  // Push Paused on top of the stack
            MessageBus::Get().SendImmediate(PausePlayModeMessage());    // Send pause message

            return 0;
        }

        static int lua_GameStateExit(lua_State* L)
        {
            (void)L;
            engineState.ClearToBase();                  // Clear all layers and reset stacks
            MessageBus::Get().SendImmediate(ExitPlayModeMessage());     // Send exit message

            if (CORE) CORE->EndGameLoop();
            //exit(0);        // immediately terminate app
            return 0;
        }

        static int lua_GameStateHowToPlay(lua_State* L)
        {
            (void)L;
            engineState.PushState(EngineMode::HowToPlay);   // Push HowToPlay UI on top

            return 0;
        }

        static int lua_GameStateSettingsAudio(lua_State* L)
        {
            (void)L;
            engineState.PushState(EngineMode::Settings_Audio);    // Settings Audio UI on top

            return 0;
        }

        static int lua_GameStateSettingsGamma(lua_State* L)
        {
            (void)L;
            engineState.PushState(EngineMode::Settings_Gamma);    // Settings Gamma UI on top

            return 0;
        }

        static int lua_GameStateBackToPause(lua_State* L)
        {
            (void)L;
            engineState.PopState();                         // Pop HowToPlay (or any temporary UI) to reveal Paused

            return 0;
        }

        static int lua_GameStateBackToMenu(lua_State* L)
        {
            (void)L;
            engineState.ClearToBase();
            engineState.PushState(EngineMode::Play);
            engineState.PushState(EngineMode::Menu);

            return 0;
        }

        static int lua_GameStateEndNightPhase(lua_State* L)
        {
            // Full day loop (day + night) is complete — save stats now
            float timeLeft = GameSystem::GetData("dayTimeRemainingX100") / 100.0f;
            GameSystem::SaveDayStats(timeLeft);

            // ── STOP the night timer so it can't fire TogglePhase after DayStats is shown ──
            Entity phaseTimer = ecsInterface.GetEntityByName("PhaseTimerText");
            if (ecsInterface.IsEntityValid(phaseTimer) &&
                ecsInterface.HasComponent<TimerComponent>(phaseTimer))
            {
                ecsInterface.GetComponent<TimerComponent>(phaseTimer).Pause();
                ecsInterface.GetComponent<TimerComponent>(phaseTimer).onComplete = nullptr;
            }

            // If final day is finished, skip Upgrade and go to Win/Lose
            // Stats screen still shows first, then Win/Lose on Continue
            if (SceneInterface::GetCurrentDay() >= 5)
            {
                engineState.ClearToBase();
                engineState.PushState(EngineMode::Play);
                engineState.PushState(EngineMode::Paused);
                engineState.PushState(EngineMode::EndStats);

                InputHandler* input = InputHandler::GetInstance();
                if (input) input->SetUIFocused(true);

                return 0;
            }

            // Normal flow — show DayStats screen before Upgrade
            engineState.ClearToBase();
            engineState.PushState(EngineMode::Play);
            engineState.PushState(EngineMode::Paused);
            engineState.PushState(EngineMode::DayStats);

            InputHandler* input = InputHandler::GetInstance();
            if (input) input->SetUIFocused(true);

            return 0;
        }

        // Get current phase for checking incase certain updates shouldn't run in other phases (e.g customer_system.lua)
        static int lua_GetPhase(lua_State* L)
        {
            if (!phaseManager)
            {
                lua_pushnil(L);
                return 1;
            }

            GamePhase currentPhase = phaseManager->GetCurrentPhase();
            lua_pushinteger(L, static_cast<int>(currentPhase));
            return 1;
        }

        // Start cutscene
        static int lua_GameStateCutscene(lua_State* L)
        {
            (void)L;
            engineState.PushState(EngineMode::Cutscene);    // Cutscene UI
            return 0;
        }

        // Start Win
        static int lua_GameStateWin(lua_State* L)
        {
            (void)L;
            engineState.PushState(EngineMode::Win);    // Cutscene UI
            return 0;
        }

        // Start Lose
        static int lua_GameStateLose(lua_State* L)
        {
            (void)L;
            engineState.PushState(EngineMode::Lose);    // Cutscene UI
            return 0;
        }

        // Start credits
        static int lua_GameStateCredit(lua_State* L)
        {
            (void)L;
            engineState.PushState(EngineMode::Credit);    // Cutscene UI
            return 0;
        }

        // ============ END GAME STATE LUA BINDINGS ============


        static int Lua_GetItemName(lua_State* L) {
            int entityID = (int)lua_tointeger(L, 1);

            if (ecsInterface.IsEntityValid(entityID) &&
                ecsInterface.HasComponent<Framework::ItemComponent>(entityID))
            {
                auto& item = ecsInterface.GetComponent<Framework::ItemComponent>(entityID);
                lua_pushstring(L, item.name.c_str());
                return 1;
            }
            lua_pushstring(L, "");
            return 1;
        }

        // Get item name from item ID (item mapping to name)
        static int lua_GetItemNameById(lua_State* L) {
            int itemId = static_cast<int>(luaL_checkinteger(L, 1));

            if (itemId < 0 || itemId >= Framework::kItemCount) {
                lua_pushstring(L, "");
                return 1;
            }

            lua_pushstring(L, Framework::kItemNames[itemId]);
            return 1;
        }

        static int lua_GetItemIdByName(lua_State* L) {
            const char* name = luaL_checkstring(L, 1);
            int itemId = Framework::ItemNameToId(name);
            lua_pushinteger(L, itemId); // returns -1 if not found
            return 1;
        }

        static int lua_GetItemDisplayNameById(lua_State* L) {
            // 1. Get the internal string ID (e.g., "warmth_potion")
            const char* stringId = luaL_checkstring(L, 1);

            // 2. Convert string ID to numeric ID
            int numericId = Framework::ItemNameToId(stringId);

            // 3. Get the display name using your existing C++ helper
            if (numericId != -1) {
                std::string displayName = Framework::GetItemDisplayName(numericId);
                lua_pushstring(L, displayName.c_str());
            }
            else {
                // Fallback if ID is invalid
                lua_pushstring(L, stringId);
            }

            return 1;
        }

        int Lua_AddToInventory(lua_State* L) {
            const char* itemName = lua_tostring(L, 1); // Arg 1: "RedMushroom"

            int itemId = Framework::ItemNameToId(itemName);
            if (itemId < 0) {
                lua_pushinteger(L, -1); // Error
                return 1;
            }

            auto& inv = Framework::Inventory::Instance();
            bool added = inv.TryAddItem_BackpackFirst(itemId, 0, 1);
            lua_pushinteger(L, added ? 0 : -1); // 0 = success, -1 = full
            return 1;
        }

        int Lua_IsInventoryFull(lua_State* L) {
            bool full = Framework::Inventory::Instance().IsInventoryFull();
            lua_pushboolean(L, full);
            return 1;
        }

        static int lua_RemoveItemByName(lua_State* L) {
            const char* itemName = luaL_checkstring(L, 1);
            int itemId = Framework::ItemNameToId(itemName);
            if (itemId < 0) return 0;

            auto& inv = Framework::Inventory::Instance();

            // Search hotbar
            int hotSlots = inv.GetHotbarSlotCount();
            for (int i = 0; i < hotSlots; ++i) {
                if (inv.Get(i).itemId == itemId) {
                    inv.ClearHotbarSlot(i);
                    return 0;
                }
            }

            // Search backpack
            int backSlots = inv.GetBackpackSlotCount();
            for (int i = 0; i < backSlots; ++i) {
                if (inv.GetBackpack(i).itemId == itemId) {
                    inv.GetBackpack(i) = { -1, 0, 0 };
                    return 0;
                }
            }
            return 0;
        }

        int Lua_DestroyEntity(lua_State* L) {
            int entityID = (int)lua_tointeger(L, 1);

            // Ensure we clean up physics body first if needed
            auto physics = ecsInterface.GetSystem<Framework::PhysicsSystem>();
            if (physics) physics->RemoveBodyForEntity(entityID);

            ecsInterface.DestroyEntity(entityID);
            return 0;
        }

        // 1. Define the function
        static int lua_IsEntityValid(lua_State* L) {
            Entity e = static_cast<Entity>(luaL_checkinteger(L, 1));
            bool valid = ecsInterface.IsEntityValid(e);
            lua_pushboolean(L, valid);
            return 1;
        }

        static int Lua_IsItemDestroyOnPick(lua_State* L) {
            int entityID = (int)lua_tointeger(L, 1);

            if (ecsInterface.IsEntityValid(entityID) &&
                ecsInterface.HasComponent<Framework::ItemComponent>(entityID))
            {
                auto& item = ecsInterface.GetComponent<Framework::ItemComponent>(entityID);
                lua_pushboolean(L, item.toDestroyWhenPicked); // Return the bool
                return 1;
            }
            lua_pushboolean(L, false);
            return 1;
        }

        int Lua_SpawnZipPickup(lua_State* L) {
            const char* itemName = lua_tostring(L, 1);
            float x = (float)lua_tonumber(L, 2);
            float y = (float)lua_tonumber(L, 3);
            float z = (float)lua_tonumber(L, 4);

            int itemId = Framework::ItemNameToId(itemName);
            if (itemId < 0) return 0;

            auto playerInteract = ecsInterface.GetSystem<PlayerInteractSystem>();
            if (playerInteract)
                playerInteract->SpawnZipPickup(itemId, glm::vec3(x, y, z));

            return 0;
        }

        // --------- Scene Loading --------------
        static int Lua_RegisterButtonCallback(lua_State* L)
        {
            int entity = (int)luaL_checkinteger(L, 1);

            if (!lua_isfunction(L, 2))
                return luaL_error(L, "Second argument must be a function");
            // 1. Function to get the ID of the item currently held


                // Create reference to function
            lua_pushvalue(L, 2);
            int funcRef = luaL_ref(L, LUA_REGISTRYINDEX);

            if (!ecsInterface.HasComponent<ButtonComponent>(entity))
                return luaL_error(L, "Entity %d has no ButtonComponent", entity);

            auto& btn = ecsInterface.GetComponent<ButtonComponent>(entity);

            // Capture lua_State* and entity by value, because onClick is void()
            lua_State* L_state = L;

            btn.onClick = [L_state, funcRef, entity]()
                {
                    // Push the stored Lua function
                    lua_rawgeti(L_state, LUA_REGISTRYINDEX, funcRef);

                    // Pass the entity id as a Lua argument
                    lua_pushinteger(L_state, (lua_Integer)entity);

                    if (lua_pcall(L_state, 1, 0, 0) != LUA_OK)
                    {
                        std::cout << "[Lua] Button callback error: "
                            << lua_tostring(L_state, -1) << "\n";
                        lua_pop(L_state, 1); // pop error message
                    }
                };

            return 0;
        }

        static int lua_SetScale(lua_State* L)
        {
            // 1. Get Entity ID
            Entity e = static_cast<Entity>(luaL_checkinteger(L, 1));

            // 2. Get X, Y, Z scales
            // Lua arguments start from index 2
            float x = static_cast<float>(luaL_checknumber(L, 2));
            float y = static_cast<float>(luaL_checknumber(L, 3));
            float z = static_cast<float>(luaL_checknumber(L, 4));

            // 3. Validate Entity & Component
            if (!ecsInterface.IsEntityValid(e) || !ecsInterface.HasComponent<TransformComponent>(e))
            {
                return luaL_error(L, "SetScale: invalid entity %d or missing TransformComponent", (int)e);
            }

            // 4. Apply Scale
            auto& tr = ecsInterface.GetComponent<TransformComponent>(e);
            tr.scale = glm::vec3(x, y, z);
            return 0;
        }


        int Lua_GetPlayerSelectedItemId(lua_State* L) {
            int itemId = Framework::Inventory::Instance().GetSelectedItemId();
            // In your Inventory.cpp, -1 (unsigned -1) is empty. 
            // Let's ensure we return -1 to Lua if it's empty/max_uint
            if (itemId == (unsigned)-1) itemId = -1;

            lua_pushinteger(L, itemId);
            return 1; // Returns 1 value to Lua
        }

        // 2. Function to remove the item currently held (consumes it)
        int Lua_ConsumePlayerSelectedItem(lua_State* L) {
            auto& inv = Framework::Inventory::Instance();
            int idx = inv.GetSelectedIndex();

            // Clear the slot
            inv.ClearHotbarSlot(idx);

            return 0; // Returns nothing
        }

        static int lua_SetParticleEmitterActive(lua_State* L)
        {
            // Arg 1: Entity ID
            Entity e = static_cast<Entity>(luaL_checkinteger(L, 1));
            // Arg 2: Boolean state
            bool active = lua_toboolean(L, 2);

            if (ecsInterface.IsEntityValid(e) && ecsInterface.HasComponent<ParticleEmitterComponent>(e))
            {
                auto& emitter = ecsInterface.GetComponent<ParticleEmitterComponent>(e);
                emitter.active = active;
            }
            return 0;
        }

        static int Lua_LoadScene(lua_State* L)
        {
            int argc = lua_gettop(L);
            if (argc < 1 || !lua_isstring(L, 1))
                return luaL_error(L, "LoadScene(path[, additive]) expects at least a string");

            const char* path = lua_tostring(L, 1);
            bool additive = false;

            if (argc >= 2)
                additive = lua_toboolean(L, 2) != 0;

            // Use the global sceneManager instance
            sceneManager.LoadScene(path, additive);

            return 0; // no return values
        }

        // --------- Animation --------------
        // Helper to play an animation on an entity
        static int l_PlayEntityAnimation(lua_State* L) {
            // 1. Get Arguments: Entity ID and Animation Name
            Entity entity = static_cast<Entity>(luaL_checkinteger(L, 1));
            const char* animName = luaL_checkstring(L, 2);

            // 2. Check if entity has animation component
            if (ecsInterface.IsEntityValid(entity) && ecsInterface.HasComponent<AnimationProperties>(entity)) {
                auto& animProps = ecsInterface.GetComponent<AnimationProperties>(entity);

                // 3. Call the member function you wanted to use!
                animProps.PlayAnimation(animName);

                // Optional: Force restart if it's already playing
                animProps.currentTime = 0.0f;
            }
            else {
                std::cout << "[Lua Warning] PlayAnimation called on invalid entity or missing AnimationProperties: " << entity << std::endl;
            }
            return 0;
        }

        // Helper to configure animation properties
        static int l_SetAnimationSettings(lua_State* L) {
            // 1. Get Arguments
            Entity entity = static_cast<Entity>(luaL_checkinteger(L, 1));
            float speed = (float)luaL_checknumber(L, 2);
            bool loop = lua_toboolean(L, 3);
            bool playOnStart = lua_toboolean(L, 4);

            // 2. Check for Component
            if (ecsInterface.IsEntityValid(entity) && ecsInterface.HasComponent<AnimationProperties>(entity)) {
                auto& animProps = ecsInterface.GetComponent<AnimationProperties>(entity);

                // 3. Apply Settings
                animProps.animationSpeed = speed;
                animProps.bLoops = loop;
                animProps.bPlayOnStart = playOnStart;
            }
            return 0;
        }

        // Lua Call: int CheckValidRecipe(table ingredientIDs)
        // Returns: -1 if invalid, or the ItemID (int) of the result if valid
        static int l_CheckValidRecipe(lua_State* L) {
            // 1. Validate Input
            if (!lua_istable(L, 1)) {
                lua_pushinteger(L, -1); // Return -1 (Error/Invalid)
                return 1;
            }

            std::vector<int> cauldronIngredients;

            // 2. Extract Ingredients from Lua Table
            lua_pushnil(L);
            while (lua_next(L, 1) != 0) {
                if (lua_isnumber(L, -1)) {
                    cauldronIngredients.push_back((int)lua_tonumber(L, -1));
                }
                lua_pop(L, 1);
            }

            // 3. Sort input ingredients so order doesn't matter
            std::sort(cauldronIngredients.begin(), cauldronIngredients.end());

            // 4. Check against Global Recipes
            const auto& allRecipes = Framework::GetGlobalRecipes();
            int foundResultID = -1; // Default to -1 (No Match Found)

            for (const auto& recipe : allRecipes) {
                // Copy recipe ingredients to sort them for comparison
                std::vector<int> recipeIngredients = recipe.ingredientIDs;
                std::sort(recipeIngredients.begin(), recipeIngredients.end());

                // Compare the vectors
                if (cauldronIngredients == recipeIngredients) {
                    foundResultID = recipe.resultItemID; // [FIX] Capture the Result ID
                    break;
                }
            }

            // 5. Return the ID to Lua
            // If valid: returns e.g. 4 (Vitality Potion ID)
            // If invalid: returns -1
            lua_pushinteger(L, foundResultID);
            return 1;
        }

        static int lua_GetItemValue(lua_State* L) {
            // 1. Get the itemId from the first argument passed in Lua
            int itemId = static_cast<int>(luaL_checkinteger(L, 1));

            // 2. Access the database using your helper function in ItemComponent.h
            // This will return the goldValue loaded from your JSON
            int value = Framework::GetItemValue(itemId);

            // 3. Push the result back to Lua
            lua_pushinteger(L, value);

            // Return 1 because we are returning 1 value to Lua
            return 1;
        }

        // REGISTER IN SCRIPT SYSTEM
        // lua_register(L, "CheckValidRecipe", l_CheckValidRecipe);

        // NavMesh Exposed funcs for path-finding

        void ScriptPathfindTo(const std::string& agentName, const std::string& targetName)
        {
            auto& ecs = ecsInterface;
            auto navSystem = ecs.GetSystem<Framework::NavMeshSystem>();
            if (!navSystem)
                return;

            Entity agent = ecs.GetEntityByName(agentName);
            Entity target = ecs.GetEntityByName(targetName);

            if (!ecs.IsEntityValid(agent) || !ecs.IsEntityValid(target))
            {
                std::cerr << "[Lua] Invalid agent or target name!" << std::endl;
                return;
            }

            bool missing = false;

            if (!ecs.HasComponent<NavMeshAgentComponent>(agent)) {
                std::cerr << "[Lua] Missing NavMeshAgentComponent on agent!" << std::endl;
                missing = true;
            }

            if (!ecs.HasComponent<TransformComponent>(agent)) {
                std::cerr << "[Lua] Missing TransformComponent on agent!" << std::endl;
                missing = true;
            }

            if (!ecs.HasComponent<TransformComponent>(target)) {
                std::cerr << "[Lua] Missing TransformComponent on target!" << std::endl;
                missing = true;
            }

            if (missing) {
                return;
            }


            auto& agentXform = ecs.GetComponent<TransformComponent>(agent);
            auto& targetXform = ecs.GetComponent<TransformComponent>(target);

            glm::vec3 from = navSystem->GetNearestPointOnNavmesh(agentXform.position);
            glm::vec3 to = navSystem->GetNearestPointOnNavmesh(targetXform.position);

            std::cout << "[Lua] Pathfinding from '" << agentName << "' to '" << targetName << "'..." << std::endl;

            navSystem->SetAgentDestination(agent, to);
        }

        static int lua_PathfindToPosition(lua_State* L) {
            // Get entity (can be ID or name)
            Entity agent = 0;

            if (lua_isnumber(L, 1)) {
                // Direct entity ID
                agent = static_cast<Entity>(luaL_checkinteger(L, 1));
            }
            else if (lua_isstring(L, 1)) {
                // Entity name - look it up
                const char* agentName = lua_tostring(L, 1);
                agent = ecsInterface.GetEntityByName(agentName);
            }
            else {
                return luaL_error(L, "PathfindToPos: first argument must be entity ID or name");
            }

            // Get destination coordinates
            float x = static_cast<float>(luaL_checknumber(L, 2));
            float y = static_cast<float>(luaL_checknumber(L, 3));
            float z = static_cast<float>(luaL_checknumber(L, 4));

            // Validate entity
            if (!ecsInterface.IsEntityValid(agent)) {
                return luaL_error(L, "PathfindToPos: invalid entity");
            }

            // Get NavMeshSystem
            auto navSystem = ecsInterface.GetSystem<Framework::NavMeshSystem>();
            if (!navSystem) {
                return luaL_error(L, "PathfindToPos: NavMeshSystem not available");
            }

            // Snap destination to navmesh (ensures valid position)
            glm::vec3 destination = navSystem->GetNearestPointOnNavmesh(glm::vec3(x, y, z));

            // Set agent destination
            navSystem->SetAgentDestination(agent, destination);

            // Return success
            lua_pushboolean(L, true);
            return 1;
        }

        //Potion registry recipe check
        static int lua_GetCraftablePotions(lua_State* L)
        {
            auto craftable = Framework::GetCraftablePotionNames();

            lua_newtable(L);
            for (size_t i = 0; i < craftable.size(); ++i) {
                lua_pushstring(L, craftable[i].c_str());
                lua_rawseti(L, -2, static_cast<int>(i + 1)); // Lua arrays start at 1
            }

            return 1; // Return the table
        }

        static int lua_GetRandomCraftablePotion(lua_State* L)
        {
            auto craftable = Framework::GetCraftablePotionNames();

            if (craftable.empty()) {
                lua_pushnil(L);
                return 1;
            }

            // Pick random
            int idx = rand() % craftable.size();
            lua_pushstring(L, craftable[idx].c_str());
            return 1; // Return the potion name string
        }
  
        static int lua_GetRandomInventoryPotion(lua_State* L)
        {
            // Collect names of all potions currently in the player's inventory
            // (hotbar + backpack). Returns a random one, or nil if none found.
            auto& inv = Framework::Inventory::Instance();
            auto& itemDB = Framework::GetItemDatabase();

            std::vector<std::string> potionsInInventory;

            // --- Hotbar ---
            int hotbarCount = inv.GetHotbarSlotCount();
            for (int i = 0; i < hotbarCount; ++i)
            {
                const auto& slot = inv.Get(i);
                if (slot.itemId < 0 || slot.count <= 0) continue;

                // Look up item definition to check type
                for (const auto& def : itemDB)
                {
                    if (def.type == Framework::ItemType::POTION && def.id == Framework::kItemNames[slot.itemId])
                    {
                        potionsInInventory.push_back(def.id);
                        break;
                    }
                }
            }

            // --- Backpack ---
            int backpackCount = inv.GetBackpackSlotCount();
            for (int i = 0; i < backpackCount; ++i)
            {
                const auto& slot = inv.GetBackpack(i);
                if (slot.itemId < 0 || slot.count <= 0) continue;

                for (const auto& def : itemDB)
                {
                    if (def.type == Framework::ItemType::POTION && def.id == Framework::kItemNames[slot.itemId])
                    {
                        potionsInInventory.push_back(def.id);
                        break;
                    }
                }
            }

            if (potionsInInventory.empty())
            {
                lua_pushnil(L);
                return 1;
            }

            // Pick a random potion from the list
            int idx = rand() % potionsInInventory.size();
            lua_pushstring(L, potionsInInventory[idx].c_str());
            return 1;
        }

        static int lua_GetEntityName(lua_State* L)
        {
            // 1. Get the Entity ID from the first argument passed in Lua
            // (Lua stack index 1 is the first argument)
            int entityID = (int)lua_tointeger(L, 1);
            auto& ecs = ecsInterface;
            // Optional: Check validity before getting name to prevent crashes
            // if (!ecs.IsEntityValid(entityID)) {
            //     lua_pushstring(L, "Invalid_Entity");
            //     return 1; 
            // }

            // 3. Get the name string
            std::string name = ecs.GetEntityName(entityID);

            // 4. Push the string onto the Lua stack
            lua_pushstring(L, name.c_str());

            // 5. Return 1 because we are returning one value (the string)
            return 1;
        }

        static int lua_SetEntityName(lua_State* L)
        {
            int entityID = (int)lua_tointeger(L, 1);
            const char* newName = luaL_checkstring(L, 2);
            auto& ecs = ecsInterface;
            ecs.SetEntityName(entityID, std::string(newName));
            return 0;
        }

        namespace fs = std::filesystem;

        // Join a relative path with fallback prefixes until a file is found.
        static std::string resolve_path(const std::string& rel) {
            // First try from project root
            fs::path fromRoot = fs::path(ScriptingSystem::GetProjectRoot()) / rel;
            std::error_code ec;
            if (fs::exists(fromRoot, ec)) {
                return fs::weakly_canonical(fromRoot, ec).string();
            }

            // Fallback to old behavior for backwards compatibility
            const char* roots[] = { ".", "..", "../.." };
            for (auto r : roots) {
                fs::path p = fs::path(r) / rel;
                if (fs::exists(p, ec)) return fs::weakly_canonical(p, ec).string();
            }

            // If not found, return as-is
            return rel;
        }

        static bool prepare_output_and_skip_if_fresh(const std::string& jsonPath,
            const std::string& outPath) {
            namespace fs = std::filesystem;
            try {
                fs::path outp(outPath);
                if (!outp.parent_path().empty())
                    fs::create_directories(outp.parent_path());

                if (fs::exists(outPath)) {
                    auto tJson = fs::last_write_time(jsonPath);
                    auto tBin = fs::last_write_time(outPath);
                    if (tBin >= tJson) return true; // already up-to-date
                }
            }
            catch (...) {
                // swallow; we'll just recompile
                std::cerr << "[Cook] Warning: failed to check timestamps � forcing recompile.\n";
            }
            return false;
        }

        // Cook.WindowConfig(jsonPath, outPath)
        int l_Cook_WindowConfig(lua_State* L) {
            std::string json = resolve_path(luaL_checkstring(L, 1));
            std::string out = resolve_path(luaL_checkstring(L, 2));
            std::string desc = resolve_path(luaL_checkstring(L, 3));

            bool skipped = prepare_output_and_skip_if_fresh(json, out);
            bool ok = skipped ? true : CompileSingleObject<WindowConfig>(json, out, desc, g_WindowConfigProps);
            lua_pushboolean(L, ok);
            return 1;
        }

        // This function implements the "Is
        // ButtonDown(button)" for Lua
        static int l_IsMouseButtonDown(lua_State* L) {
            // Get the 'self' object (ScriptingSystem or ScriptInstance)
            auto* self = lua_self(L);
            Graphics* g = self ? self->GetGraphicsSystem() : nullptr;
            if (!g) {
                lua_pushboolean(L, 0);
                return 1; // Return 1 value (the boolean false)
            }
            GLFWwindow* win = g->GetWindow();
            int button = (int)luaL_checkinteger(L, 1);
            int down = (win && glfwGetMouseButton(win, button) == GLFW_PRESS) ? 1 : 0;
            lua_pushboolean(L, down);
            return 1;
        }

        // Cook.Audio(jsonPath, outPath [, key="AudioProperties"])
        int l_Cook_Audio(lua_State* L) {
            const char* json = luaL_checkstring(L, 1);
            const char* out = luaL_checkstring(L, 2);
            const char* desc = luaL_checkstring(L, 3);
            const char* key = luaL_optstring(L, 4, "AudioProperties");

            bool skipped = prepare_output_and_skip_if_fresh(json, out);
            bool ok = skipped ? true : CompileArray<AudioBankMetadata>(json, out, desc, g_AudioBankMetadataProps, key);
            lua_pushboolean(L, ok);
            return 1;
        }

        // Cook.Textures(jsonPath, outPath [, key="textures"])
        int l_Cook_Textures(lua_State* L) {
            const char* json = luaL_checkstring(L, 1);
            const char* out = luaL_checkstring(L, 2);
            const char* desc = luaL_checkstring(L, 3);
            const char* key = luaL_optstring(L, 4, "textures");

            bool skipped = prepare_output_and_skip_if_fresh(json, out);
            bool ok = skipped ? true : CompileArray<TextureProperties>(json, out, desc, g_TexturePropertiesProps, key);
            lua_pushboolean(L, ok);
            return 1;
        }

        // Cook.Meshes(jsonPath, outPath [, key="mesh"])
        int l_Cook_Meshes(lua_State* L) {
            const char* json = luaL_checkstring(L, 1);
            const char* out = luaL_checkstring(L, 2);
            const char* desc = luaL_checkstring(L, 3);
            const char* key = luaL_optstring(L, 4, "mesh");

            bool skipped = prepare_output_and_skip_if_fresh(json, out);
            bool ok = skipped ? true : CompileArray<MeshProperties>(json, out, desc, g_MeshProps, key);
            lua_pushboolean(L, ok);
            return 1;
        }

        // Cook.Prefab(jsonPath, outPath [, key="prefabs"])
        int l_Cook_Prefab(lua_State* L) {
            const char* json = luaL_checkstring(L, 1);
            const char* out = luaL_checkstring(L, 2);
            const char* desc = luaL_checkstring(L, 3);
            const char* key = luaL_optstring(L, 4, "prefabs");

            bool skipped = prepare_output_and_skip_if_fresh(json, out);
            bool ok = skipped ? true : CompileArray<PrefabProperties>(json, out, desc, g_PrefabProps, key);
            lua_pushboolean(L, ok);
            return 1;
        }

        // Cook.Scene(jsonPath, outPath [, key="scenes"])
        int l_Cook_Scene(lua_State* L) {
            const char* json = luaL_checkstring(L, 1);
            const char* out = luaL_checkstring(L, 2);
            const char* desc = luaL_checkstring(L, 3);
            const char* key = luaL_optstring(L, 4, "scenes");

            bool skipped = prepare_output_and_skip_if_fresh(json, out);
            bool ok = skipped ? true : CompileArray<SceneProperties>(json, out, desc, g_SceneProps, key);
            lua_pushboolean(L, ok);
            return 1;
        }

        // Cook.Scene(jsonPath, outPath [, key="materials"])
        int l_Cook_Material(lua_State* L) {
            const char* json = luaL_checkstring(L, 1);
            const char* out = luaL_checkstring(L, 2);
            const char* desc = luaL_checkstring(L, 3);
            const char* key = luaL_optstring(L, 4, "materials");

            bool skipped = prepare_output_and_skip_if_fresh(json, out);
            bool ok = skipped ? true : CompileArray<MaterialProperties>(json, out, desc, g_MaterialProps, key);
            lua_pushboolean(L, ok);
            return 1;
        }

        // Check if an entity has ItemComponent and if it is choppable
        static int lua_IsItemChoppable(lua_State* L)
        {
            // 1. Get Entity ID from Lua
            Entity e = static_cast<Entity>(luaL_checkinteger(L, 1));

            // 2. Validate Entity
            if (!ecsInterface.IsEntityValid(e))
            {
                lua_pushboolean(L, false);
                return 1;
            }

            // 3. Check if it has the ItemComponent
            if (ecsInterface.HasComponent<ItemComponent>(e))
            {
                auto& item = ecsInterface.GetComponent<ItemComponent>(e);
                // 4. Return the bool value
                lua_pushboolean(L, item.isChoppable);
            }
            else
            {
                // Component doesn't exist, so it's definitely not choppable
                lua_pushboolean(L, false);
            }

            return 1; // Returning 1 value (boolean)
        }

        static uint64_t FileTimeToTicks(const std::filesystem::file_time_type& ft) {
            return std::chrono::duration_cast<std::chrono::milliseconds>(ft.time_since_epoch()).count();
        }

        // Helper to find the GUID string from a friendly name
        static int l_GetAnimationGUID(lua_State* L)
        {
            const char* name = luaL_checkstring(L, 1);
            // Ask AssetManager for the GUID of the animation with this name
            AssetHandle guid = AssetManager::Instance().FindGuid<AnimationProperties>(name);

            if (guid.id.empty()) {
                std::cout << "[Lua Warning] Animation Asset not found: " << name << std::endl;
            }

            lua_pushstring(L, guid.id.c_str());
            return 1; // Return the GUID string to Lua
        }

        // [NEW] Helper to swap the Animation Asset by GUID
        static int l_SetAnimationAsset(lua_State* L)
        {
            Entity e = static_cast<Entity>(luaL_checkinteger(L, 1));
            const char* guidStr = luaL_checkstring(L, 2);

            if (ecsInterface.IsEntityValid(e) && ecsInterface.HasComponent<AnimationProperties>(e))
            {
                auto& anim = ecsInterface.GetComponent<AnimationProperties>(e);
                AssetHandle newGuid(guidStr);

                // Only swap if it's different
                if (anim.guid.id != newGuid.id)
                {
                    anim.guid = newGuid;

                    // Load the new data
                    AnimationProperties* asset = AssetManager::Instance().Get<AnimationProperties>(newGuid);
                    if (asset)
                    {
                        // Ensure data is loaded
                        if (asset->skeleton.bones.empty()) {
                            AnimationLoader::loadAnimation(asset->path, *asset);
                        }

                        // Copy the new skeleton and clips
                        anim.skeleton = asset->skeleton;
                        anim.availableAnimations = asset->availableAnimations;
                        anim.globalInverseTransform = asset->globalInverseTransform;

                        // Reset Playback
                        anim.currentTime = 0.0f;
                        anim.currentAnimation = nullptr;
                        anim.StartDefaultAnimation();

                        // Resize matrices for safety
                        anim.finalBoneMatrices.resize(anim.skeleton.bones.size(), glm::mat4(1.0f));

                        // std::cout << "[Lua] Swapped Animation Asset to: " << asset->name << std::endl;
                    }
                }
            }
            return 0;
        }
        static int lua_
        (lua_State* L)
        {
            // 1. Get Entity ID
            Entity e = static_cast<Entity>(luaL_checkinteger(L, 1));

            // 2. Get X, Y, Z scales
            // Lua arguments start from index 2
            float x = static_cast<float>(luaL_checknumber(L, 2));
            float y = static_cast<float>(luaL_checknumber(L, 3));
            float z = static_cast<float>(luaL_checknumber(L, 4));

            // 3. Validate Entity & Component
            if (!ecsInterface.IsEntityValid(e) || !ecsInterface.HasComponent<TransformComponent>(e))
            {
                return luaL_error(L, "SetScale: invalid entity %d or missing TransformComponent", (int)e);
            }

            // 4. Apply Scale
            auto& tr = ecsInterface.GetComponent<TransformComponent>(e);
            tr.scale = glm::vec3(x, y, z);
            return 0;
        }

        // Helper to get the NAME of the currently selected item
        static int Lua_GetSelectedItemName(lua_State* L) {
            auto& inv = Framework::Inventory::Instance();
            int idx = inv.GetSelectedIndex();
            const auto& slot = inv.Get(idx);

            // Check if slot has an item and if the ID is safe
            if (slot.itemId >= 0 && slot.itemId < Framework::kItemCount) {
                // Access the array directly from ItemComponent.h
                lua_pushstring(L, Framework::kItemNames[slot.itemId]);
            }
            else {
                lua_pushnil(L); // Return nil if empty
            }
            return 1; // Returning 1 value (string or nil)
        }

        // In ScriptingSystem.cpp inside anonymous namespace

        static void StopAnimation(Entity entityID)
        {
            // Check if the entity has animation properties
            if (ecsInterface.HasComponent<AnimationProperties>(entityID))
            {
                // Get the component
                auto& anim = ecsInterface.GetComponent<AnimationProperties>(entityID);

                // "Stop" by setting speed to 0 (Freezes the frame)
                anim.animationSpeed = 0.0f;
            }
        }
        static int lua_GetScreenSize(lua_State* L) {
            auto* self = lua_self(L);
            Graphics* g = self ? self->GetGraphicsSystem() : nullptr;
            if (!g) {
                lua_pushnumber(L, 1920.0);
                lua_pushnumber(L, 1080.0);
                return 2;
            }

            GLFWwindow* win = g->GetWindow();
            int width = 1920, height = 1080;
            if (win) {
                glfwGetWindowSize(win, &width, &height);
            }

            lua_pushnumber(L, width);
            lua_pushnumber(L, height);
            return 2; // Returns width and height to Lua
        }
        static int lua_GetRotation(lua_State* L)
        {
            Entity e = static_cast<Entity>(luaL_checkinteger(L, 1));

            if (!ecsInterface.IsEntityValid(e) ||
                !ecsInterface.HasComponent<TransformComponent>(e))
            {
                return luaL_error(L, "GetRotation: entity %d has no TransformComponent", (int)e);
            }

            auto& tr = ecsInterface.GetComponent<TransformComponent>(e);
            lua_pushnumber(L, tr.rotation.x); // Pitch
            lua_pushnumber(L, tr.rotation.y); // Yaw
            lua_pushnumber(L, tr.rotation.z); // Roll
            return 3;
        }

        // In ScriptingSystem.cpp, inside the anonymous namespace
        static int lua_SetCameraRot(lua_State* L)
        {
            float pitch = (float)luaL_checknumber(L, 1);
            float yaw = (float)luaL_checknumber(L, 2);

            auto* gfx = Graphics::GetInstance();
            if (gfx) {
                Camera* cam = gfx->GetCameraByIndex(0); // 0 is the GameCamera
                if (cam) {
                    cam->pitch = pitch;
                    cam->yaw = yaw;

                    // THIS recalculates the front/up/right vectors!
                    cam->updateVectors();
                }
            }
            return 0;
        }

        static int lua_GetCameraRot(lua_State* L) {
            Camera* cam = Graphics::GetInstance()->GetCameraByIndex(0);
            if (cam) {
                lua_pushnumber(L, cam->pitch);
                lua_pushnumber(L, cam->yaw);
                return 2; // Returning two values to Lua
            }
            return 0;
        }

        static int lua_GetCameraYaw(lua_State* L)
        {
            // 1. Get the Graphics System
            auto* gfx = Graphics::GetInstance();
            if (!gfx) {
                lua_pushnumber(L, 0.0f);
                return 1;
            }

            // 2. Get the GAME CAMERA specifically (Index 0)
            // Index 1 would be the Scene/Editor camera
            Camera* cam = gfx->GetCameraByIndex(0);

            if (!cam) {
                lua_pushnumber(L, 0.0f);
                return 1;
            }

            // 3. Return the Yaw (Horizontal Rotation)
            lua_pushnumber(L, cam->yaw);
            return 1;
        }

        static int lua_TogglePhase(lua_State* L)
        {
            // Assuming PhaseManager is a singleton or accessible via ecsInterface/ServiceLocator
            // Replace 'PhaseManager::Instance()' with your actual access method
            phaseManager->TogglePhase();
            return 0;
        }

        static int lua_GetGamma(lua_State* L)
        {
            auto* gfx = Graphics::GetInstance();
            if (!gfx) { lua_pushnumber(L, 2.2); return 1; }
            lua_pushnumber(L, gfx->GetGamma());
            return 1;
        }

        static int lua_SetGamma(lua_State* L)
        {
            float g = (float)luaL_checknumber(L, 1);
            auto* gfx = Graphics::GetInstance();
            if (gfx) gfx->SetGamma(g);
            return 0;
        }

        // LUA DAY PHASE SCENES //
        static int lua_AdvanceDayAndPushTransition(lua_State* L)
        {
            SceneInterface::AdvanceToNextDay();
            SceneInterface::PushDayTransitionStateForCurrentDay();
            return 0; // no values returned to Lua
        }
        
        //FOR ZIPPING ITEMS
        static int lua_TriggerZipPickup(lua_State* L)
        {
            // 1. Extract arguments from Lua
            const char* itemName = luaL_checkstring(L, 1);
            float x = static_cast<float>(luaL_checknumber(L, 2));
            float y = static_cast<float>(luaL_checknumber(L, 3));
            float z = static_cast<float>(luaL_checknumber(L, 4));

            // 2. Convert name to ID
            int itemId = Framework::ItemNameToId(itemName);

            // 3. Trigger the C++ system if valid
            if (itemId >= 0)
            {
                auto interactSystem = ecsInterface.GetSystem<Framework::PlayerInteractSystem>();
                if (interactSystem)
                {
                    interactSystem->SpawnZipPickup(itemId, glm::vec3(x, y, z));
                }
            }
            else
            {
                std::cout << "[ScriptingSystem] Warning: Invalid item name passed to TriggerZipPickup: " << itemName << std::endl;
            }

            return 0; // Return nothing to Lua
        }

        // Returns the entity ID the player's camera is currently pointing at (up to mLookRange).
        // Returns 0 if nothing is hit. Used by Lua to control customer textbox opacity.
        static int lua_GetLookedAtEntity(lua_State* L)
        {
            auto playerInteract = ecsInterface.GetSystem<PlayerInteractSystem>();
            if (!playerInteract)
            {
                lua_pushinteger(L, 0);
                return 1;
            }
            lua_pushinteger(L, static_cast<lua_Integer>(playerInteract->GetLookAtEntity()));
            return 1;
        }
    }

    // ---------------------------- class methods --------------------------------

    ScriptingSystem::ScriptingSystem() : L(nullptr) {}
    ScriptingSystem::~ScriptingSystem() { if (L) { lua_close(L); L = nullptr; } }

    void ScriptingSystem::Initialize() {
        // 1) Register ScriptComponent (your engine�s pattern)
        ecsInterface.RegisterComponent<ScriptEntry>();
        ecsInterface.RegisterComponent<ScriptProperties>();

        // 2) Set system signature
        Signature sig;
        sig.set(ecsInterface.GetComponentType<ScriptProperties>(), true);
        ecsInterface.SetSystemSignature<ScriptingSystem>(sig);
        std::cout << "Signature for Scripting system is: " << sig << std::endl;

        // 3) Boot Lua
        L = luaL_newstate();
        luaL_openlibs(L);

        // 4) Expose `this` to Lua (so C functions can find the system)
        lua_pushlightuserdata(L, this);
        lua_setfield(L, LUA_REGISTRYINDEX, kRegistryKey);

        // 5) Register C functions (global names in Lua)
        lua_register(L, "IsKeyDown", l_IsKeyDown);
        lua_register(L, "CameraMove", l_CameraMove);
        lua_register(L, "GetMousePosition", lua_GetMousePosition);
        lua_register(L, "GetMouseUV", lua_GetMouseUV);
        lua_register(L, "DestroyEntity", l_DestroyEntity);
        lua_register(L, "IsMouseButtonDown", l_IsMouseButtonDown);
        lua_register(L, "SetRotation", lua_SetRotation);
        lua_register(L, "SetMesh", lua_SetMesh);
        lua_register(L, "SetTexture", lua_SetTexture);
        lua_register(L, "SetAnimationSettings", l_SetAnimationSettings);
        lua_register(L, "LoadScene", Lua_LoadScene);

        //Components get & sets
        lua_register(L, "GetPosition", lua_GetPosition); // Transform get setter
        lua_register(L, "SetPosition", lua_SetPosition);

        lua_register(L, "GetTextComponent", lua_GetTextComponent); // Text component get/set
        lua_register(L, "SetTextComponent", lua_SetTextComponent);
        lua_register(L, "SetTransformComponent", lua_SetTransformComponent);

        // Game utility
        lua_register(L, "IsItemChoppable", lua_IsItemChoppable);
        lua_register(L, "SetText", lua_SetText);
        lua_register(L, "GetGamma", lua_GetGamma);
        lua_register(L, "SetGamma", lua_SetGamma);

        // Data Entry from engine
        lua_register(L, "GetScore", lua_GetScore);
        lua_register(L, "SetScore", lua_SetScore);
        lua_register(L, "GetData", lua_GetData);
        lua_register(L, "SetData", lua_SetData);
        lua_register(L, "AddData", lua_AddData);
        lua_register(L, "HasData", lua_HasData);

        lua_register(L, "DestroyEntity", Lua_DestroyEntity);
        lua_register(L, "GetItemName", Lua_GetItemName);
        lua_register(L, "RegisterButtonCallback", Lua_RegisterButtonCallback);

        lua_register(L, "SpawnCauldronItem", l_spawnCauldronItem);
        lua_register(L, "IsItemZipping", l_isItemZipping);
        lua_register(L, "CheckValidRecipe", l_CheckValidRecipe);
        lua_register(L, "GetEntityName", lua_GetEntityName);
        lua_register(L, "SetEntityName", lua_SetEntityName);

        lua_register(L, "SetScale", lua_SetScale);
        lua_register(L, "GetRotation", lua_GetRotation);
        lua_register(L, "GetCameraYaw", lua_GetCameraYaw);
        lua_register(L, "SetCameraRot", lua_SetCameraRot);
        lua_register(L, "TogglePhase", lua_TogglePhase);
		lua_register(L, "GetCameraRot", lua_GetCameraRot);

        // Inventory Utility
        lua_register(L, "IsItemDestroyOnPick", Lua_IsItemDestroyOnPick);
        lua_register(L, "GetSelectedItem", Lua_GetPlayerSelectedItemId);
        lua_register(L, "ConsumeSelectedItem", Lua_ConsumePlayerSelectedItem);
        lua_register(L, "AddToInventory", Lua_AddToInventory);
        lua_register(L, "RemoveItemByName", lua_RemoveItemByName);
        lua_register(L, "GetItemNameById", lua_GetItemNameById);
        lua_register(L, "GetSelectedItemName", Lua_GetSelectedItemName);
        lua_register(L, "SetInteractionText", l_SetInteractionText);
        lua_register(L, "GetItemIdByName", lua_GetItemIdByName);
		lua_register(L, "GetItemValue", lua_GetItemValue);
        lua_register(L, "TriggerZipPickup", lua_TriggerZipPickup);
		lua_register(L, "GetItemDisplayName", lua_GetItemDisplayNameById);
        lua_register(L, "IsInventoryFull", Lua_IsInventoryFull);

        lua_register(L, "ResetProgressOnMenuReturn", [](lua_State* L) -> int {
            UpgradeManager::Instance().ResetAll();
            std::cout << "[ScriptingSystem] ResetProgressOnMenuReturn called.\n";
            return 0;
            });

        // Engine utility
        lua_register(L, "FindEntityByName", lua_FindEntityByName);
        lua_register(L, "GetEntitiesByTag", lua_GetEntitiesByTag);
        lua_register(L, "RemoveTagFromEntity", lua_RemoveTagFromEntity);
        lua_register(L, "EntityHasTag", lua_EntityHasTag);
        lua_register(L, "IsEntityValid", lua_IsEntityValid);
        lua_register(L, "AddTagToEntity", lua_AddTagToEntity);
        lua_register(L, "PlayEntityAnimation", l_PlayEntityAnimation);
        lua_register(L, "StopAnimation", l_StopEntityAnimation);
        lua_register(L, "SpawnPrefab", lua_SpawnPrefab);
        lua_register(L, "FireEvent", lua_FireEvent); // Maybe add register if needed
        lua_register(L, "SubscribeToEvent", lua_SubscribeToEvent); // Maybe add register if needed
        lua_register(L, "SetScriptVariable", lua_SetScriptVariable);
        lua_register(L, "GetLookedAtEntity", lua_GetLookedAtEntity);
        lua_register(L, "SetParticleEmitterActive", lua_SetParticleEmitterActive);

        // Engine States
        lua_register(L, "GameStateStart",               lua_GameStateStart);
        lua_register(L, "GameStateResume",              lua_GameStateResume);
        lua_register(L, "GameStatePaused",              lua_GameStatePaused);
        lua_register(L, "GameStateExit",                lua_GameStateExit);
        lua_register(L, "GameStateHowToPlay",           lua_GameStateHowToPlay);
        lua_register(L, "GameStateBackToMenu",          lua_GameStateBackToMenu);
        lua_register(L, "GameStateCutscene",            lua_GameStateCutscene);
        lua_register(L, "GameStateWin",                 lua_GameStateWin);
        lua_register(L, "GameStateLose",                lua_GameStateLose);
        lua_register(L, "GameStateCredit",              lua_GameStateCredit);
        lua_register(L, "GameStateSettingsAudio",       lua_GameStateSettingsAudio);
        lua_register(L, "GameStateSettingsGamma",       lua_GameStateSettingsGamma);
        lua_register(L, "GameStateBackToPause",         lua_GameStateBackToPause);
        lua_register(L, "GameStateEndNightPhase",       lua_GameStateEndNightPhase);
        lua_register(L, "GetPhase",                     lua_GetPhase);
        lua_register(L, "AdvanceDayAndPushTransition",  lua_AdvanceDayAndPushTransition);

        // lua_register(L, "SetScale", lua_SetScale);
        lua_register(L, "GetScale", lua_GetScale);
        lua_register(L, "GetScreenSize", lua_GetScreenSize);
        lua_register(L, "SetIsUI", lua_SetIsUI);
        lua_register(L, "SetColliderPromptText", Lua_SetColliderPromptText);
        lua_register(L, "SpawnZipPickup", Lua_SpawnZipPickup);
        lua_register(L, "SetColliderInteractable", Lua_SetColliderInteractable);

        // Dialogue System
        lua_register(L, "DialogueClear", lua_DialogueClear);
        lua_register(L, "DialogueAdd", lua_DialogueAdd);
        lua_register(L, "DialogueStart", lua_DialogueStart);
        lua_register(L, "DialogueEnd", lua_DialogueEnd);
        lua_register(L, "DialogueIsActive", lua_DialogueIsActive);

        // Cutscenes (Camera action)
        lua_register(L, "CutsceneSetTarget", lua_CutsceneSetTarget);
        lua_register(L, "CutsceneSetOffset", lua_CutsceneSetOffset);
        lua_register(L, "CutsceneSetSpeed", lua_CutsceneSetSpeed);
        lua_register(L, "CutsceneStart", lua_CutsceneStart);
        lua_register(L, "CutsceneIsFlying", lua_CutsceneIsFlying);

        //cauldron
        lua_register(L, "SetCameraLock", [](lua_State* L) -> int {
            // 1. Get argument (true/false)
            bool locked = lua_toboolean(L, 1);

            auto* input = InputHandler::GetInstance();
            if (input) {
                // "Locked" means rotation is disabled (for minigame)
                input->SetCameraRotationEnabled(!locked);
            }
            return 0;
            });

        lua_register(L, "GetRealTime", [](lua_State* L) -> int {
            lua_pushnumber(L, (float)glfwGetTime());
            return 1;
            });

        lua_register(L, "SetGravity", [](lua_State* L) -> int {
            Entity e = static_cast<Entity>(luaL_checkinteger(L, 1));
            float gravity = static_cast<float>(luaL_checknumber(L, 2));

            if (ecsInterface.IsEntityValid(e) && ecsInterface.HasComponent<PhysicsProperties>(e))
            {
                auto& phys = ecsInterface.GetComponent<PhysicsProperties>(e);
                phys.gravityFactor = gravity;
                phys.startAwake = true; // Wake it up to apply change
            }
            return 0;
            });

        lua_register(L, "SetAnimationMode", [](lua_State* L) -> int {
            // Arg 1: Entity ID
            Entity e = static_cast<Entity>(luaL_checkinteger(L, 1));
            // Arg 2: Mode (0=Normal, 1=Stirring, 2=Manual)
            int mode = static_cast<int>(luaL_checkinteger(L, 2));

            // Validate and Apply
            if (ecsInterface.IsEntityValid(e) && ecsInterface.HasComponent<AnimationProperties>(e))
            {
                auto& anim = ecsInterface.GetComponent<AnimationProperties>(e);
                anim.updateMode = mode;
                // std::cout << "[Scripting] Set Animation Mode to " << mode << " for Entity " << e << std::endl;
            }
            return 0;
            });

        lua_register(L, "GetAnimationGUID", l_GetAnimationGUID);
        lua_register(L, "SetAnimationAsset", l_SetAnimationAsset);

        // Navmesh
        lua_register(L, "BuildNavMesh", lua_BuildNavMesh);
        lua_register(L, "PathfindTo", [](lua_State* L) -> int
            {
                const char* agentName = luaL_checkstring(L, 1);
                const char* targetName = luaL_checkstring(L, 2);

                ScriptPathfindTo(agentName, targetName);
                return 0;
            });
        lua_register(L, "PathfindToPos", lua_PathfindToPosition);

        // ============ ENGINE STATE LUA BINDINGS ============
        lua_register(L, "PushEngineState", lua_PushEngineState);
        lua_register(L, "PopEngineState", lua_PopEngineState);
        lua_register(L, "ClearEngineStates", lua_ClearEngineStates);
        lua_register(L, "GetCurrentEngineState", lua_GetCurrentEngineState);
        lua_register(L, "IsEngineState", lua_IsEngineState);
        lua_register(L, "HasEngineState", lua_HasEngineState);
        std::cout << "[ScriptingSystem] EngineState bindings registered" << std::endl;
        // ============ START MAP LUA BINDINGS ============

        // Set Entity Scale (for showing/hiding UI elements)
        lua_register(L, "SetEntityScale", [](lua_State* L) -> int {
            int entityId = (int)lua_tointeger(L, 1);
            float x = (float)lua_tonumber(L, 2);
            float y = (float)lua_tonumber(L, 3);
            float z = (float)lua_tonumber(L, 4);

            if (ecsInterface.IsEntityValid(entityId)) {
                if (ecsInterface.HasComponent<TransformComponent>(entityId)) {
                    auto& transform = ecsInterface.GetComponent<TransformComponent>(entityId);
                    transform.scale = glm::vec3(x, y, z);
                    std::cout << "[Lua] SetEntityScale: Entity " << entityId
                        << " scale set to (" << x << ", " << y << ", " << z << ")" << std::endl;
                }
            }
            else {
                std::cerr << "[Lua Error] SetEntityScale: Invalid entity " << entityId << std::endl;
            }

            return 0;
            });

        // Craftable potion queries
        lua_register(L, "GetCraftablePotions", lua_GetCraftablePotions);
        lua_register(L, "GetRandomCraftablePotion", lua_GetRandomCraftablePotion);
        lua_register(L, "GetRandomInventoryPotion", lua_GetRandomInventoryPotion);

        // Teleport player function
        lua_register(L, "TeleportPlayer", [](lua_State* L) -> int {
            float x = (float)lua_tonumber(L, 1);
            float y = (float)lua_tonumber(L, 2);
            float z = (float)lua_tonumber(L, 3);
            float rx = (float)lua_tonumber(L, 4);
            float ry = (float)lua_tonumber(L, 5);
            float rz = (float)lua_tonumber(L, 6);

            glm::vec3 pos(x, y, z);
            glm::vec3 rot(rx, ry, rz);

            if (phaseManager) {
                phaseManager->TeleportPlayer(pos, rot);
            }

            return 0;
            });

        // Check if ANY potions are craftable (returns true/false)
        lua_register(L, "HasCraftablePotions", [](lua_State* L) -> int {
            auto craftable = Framework::GetCraftablePotionNames();
            lua_pushboolean(L, !craftable.empty());
            return 1;
            });

        // Expose to Lua — returns how many unique potions are currently craftable
        lua_register(L, "GetCraftablePotionCount", [](lua_State* L) -> int {
            auto craftable = Framework::GetCraftablePotionNames();
            lua_pushinteger(L, (int)craftable.size());
            return 1;
            });

        // Get total count of a specific ingredient in player inventory
        lua_register(L, "GetIngredientCount", [](lua_State* L) -> int {
            int itemId = (int)lua_tointeger(L, 1);
            auto& inv = Framework::Inventory::Instance();
            int total = 0;
            for (int i = 0; i < inv.GetHotbarSlotCount(); ++i) {
                const auto& slot = inv.Get(i);
                if (slot.itemId == itemId) total += (slot.count > 0 ? slot.count : 1);
            }
            for (int i = 0; i < inv.GetBackpackSlotCount(); ++i) {
                const auto& slot = inv.GetBackpack(i);
                if (slot.itemId == itemId) total += (slot.count > 0 ? slot.count : 1);
            }
            lua_pushinteger(L, total);
            return 1;
            });

        // Get ingredient costs for a potion as a flat table {itemId, count, itemId, count...}
        lua_register(L, "GetPotionIngredientCost", [](lua_State* L) -> int {
            const char* potionName = luaL_checkstring(L, 1);
            int potionId = Framework::ItemNameToId(potionName);
            if (potionId < 0) { lua_newtable(L); return 1; }

            auto req = Framework::ExpandRecipeToBase(potionId);
            lua_newtable(L);
            int idx = 1;
            for (const auto& [itemId, count] : req.baseIngredients) {
                lua_newtable(L);
                lua_pushinteger(L, itemId); lua_setfield(L, -2, "itemId");
                lua_pushinteger(L, count);  lua_setfield(L, -2, "count");
                lua_rawseti(L, -2, idx++);
            }
            return 1;
            });

        lua_register(L, "GetCraftablePotionNames", [](lua_State* L) -> int {
            auto craftable = Framework::GetCraftablePotionNames();
            lua_newtable(L);
            for (int i = 0; i < (int)craftable.size(); ++i) {
                lua_pushstring(L, craftable[i].c_str());
                lua_rawseti(L, -2, i + 1);
            }
            return 1;
            });

        lua_register(L, "GetPotionDirectCost", [](lua_State* L) -> int {
            const char* potionName = luaL_checkstring(L, 1);
            int potionId = Framework::ItemNameToId(potionName);
            lua_newtable(L);
            if (potionId < 0) return 1;
            const PotionRecipe* recipe = Framework::GetRecipeForResult(potionId);
            if (!recipe) return 1;
            for (int i = 0; i < (int)recipe->ingredientIDs.size(); ++i) {
                lua_newtable(L);
                lua_pushinteger(L, recipe->ingredientIDs[i]); lua_setfield(L, -2, "itemId");
                lua_pushinteger(L, 1);                        lua_setfield(L, -2, "count");
                lua_rawseti(L, -2, i + 1);
            }
            return 1;
            });

        // Start day phase
        lua_register(L, "StartDayPhase", [](lua_State* L) -> int {
            if (phaseManager) {
                phaseManager->SetPhase(GamePhase::Day);
            }
            return 0;
            });

        // Called by DayStatsNextButton to proceed to Upgrade after stats screen
        lua_register(L, "ApplyUpgradePhase", [](lua_State* L) -> int {
            engineState.ClearToBase();
            engineState.PushState(EngineMode::Play);
            engineState.PushState(EngineMode::Upgrade);
            if (phaseManager) phaseManager->SetPhase(GamePhase::Upgrade);
            InputHandler* input = InputHandler::GetInstance();
            if (input) input->SetUIFocused(true);
            return 0;
            });

        // Returns current day index (1-5)
        lua_register(L, "GetCurrentDay", [](lua_State* L) -> int {
            lua_pushinteger(L, SceneInterface::GetCurrentDay());
            return 1;
            });

        // Returns true if player met win condition
        lua_register(L, "WinOrLose", [](lua_State* L) -> int {
            lua_pushboolean(L, SceneInterface::WinOrLose());
            return 1;
            });

        // Set UI focused (show/hide cursor)
        lua_register(L, "SetUIFocused", [](lua_State* L) -> int {
            bool focused = lua_toboolean(L, 1);

            InputHandler* input = InputHandler::GetInstance();
            if (input) {
                input->SetUIFocused(focused);
            }

            return 0;
            });

        // Call Lua function on entity
        lua_register(L, "CallLuaFunction", [](lua_State* L) -> int {
            int entityId = (int)lua_tointeger(L, 1);
            const char* funcName = lua_tostring(L, 2);

            // This requires access to ScriptingSystem instance
            // You may need to adjust based on your architecture
            auto scriptSystem = ecsInterface.GetSystem<ScriptingSystem>();
            if (scriptSystem) {
                scriptSystem->Call<void>(entityId, funcName);
            }

            return 0;
            });

        // Register button hover callback
        lua_register(L, "RegisterButtonHoverCallback", [](lua_State* L) -> int {
            int entityId = (int)lua_tointeger(L, 1);

            if (!lua_isfunction(L, 2)) {
                return luaL_error(L, "Second argument must be a function");
            }

            // Store the callback
            int callbackRef = luaL_ref(L, LUA_REGISTRYINDEX);

            // You'll need to integrate this with your button system
            // Store the callbackRef in the ButtonComponent or a separate map

            return 0;
            });
        // ============ END MAP LUA BINDINGS ============
        lua_register(L, "SetLinearVelocity", [](lua_State* L) -> int {
            Entity e = static_cast<Entity>(luaL_checkinteger(L, 1));
            float vx = static_cast<float>(luaL_checknumber(L, 2));
            float vy = static_cast<float>(luaL_checknumber(L, 3));
            float vz = static_cast<float>(luaL_checknumber(L, 4));

            if (ecsInterface.IsEntityValid(e) && ecsInterface.HasComponent<PhysicsProperties>(e))
            {
                auto& phys = ecsInterface.GetComponent<PhysicsProperties>(e);
                phys.linearVelocity = glm::vec3(vx, vy, vz);
                phys.startAwake = true; // Wake up Jolt body so it moves
            }
            return 0;
            });
        // Create Cook table with our functions
        lua_newtable(L);                           // Cook
        lua_pushcfunction(L, l_Cook_WindowConfig); lua_setfield(L, -2, "WindowConfig");
        lua_pushcfunction(L, l_Cook_Audio);        lua_setfield(L, -2, "Audio");
        lua_pushcfunction(L, l_Cook_Textures);     lua_setfield(L, -2, "Textures");
        lua_pushcfunction(L, l_Cook_Meshes);       lua_setfield(L, -2, "Meshes");
        lua_pushcfunction(L, l_Cook_Prefab);       lua_setfield(L, -2, "Prefabs");
        lua_pushcfunction(L, l_Cook_Scene);        lua_setfield(L, -2, "Scenes");
        lua_pushcfunction(L, l_Cook_Material);     lua_setfield(L, -2, "Materials");
        lua_setglobal(L, "Cook");

        lua_register(L, "PlayEntitySound", [](lua_State* L) -> int
            {
                Entity e = static_cast<Entity>(luaL_checkinteger(L, 1));
                if (g_AudioSystem) g_AudioSystem->PlayEntitySound(e);
                return 0;
            });

        lua_register(L, "SetEntityVolume", [](lua_State* L) -> int
            {
                Entity e = static_cast<Entity>(luaL_checkinteger(L, 1));
                float volume = (float)luaL_checknumber(L, 2);

                if (g_AudioSystem) g_AudioSystem->SetEntityVolume(e, volume);
                return 0;
            });

        // Create Sound table with playback functions
        lua_newtable(L);
        lua_pushcfunction(L, lua_AudioPlay);      lua_setfield(L, -2, "Play");
        lua_pushcfunction(L, lua_AudioStop);      lua_setfield(L, -2, "Stop");
        lua_pushcfunction(L, lua_AudioPause);     lua_setfield(L, -2, "Pause");
        lua_pushcfunction(L, lua_AudioResume);    lua_setfield(L, -2, "Resume");
        lua_pushcfunction(L, lua_AudioSetVolume); lua_setfield(L, -2, "SetVolume");
        lua_pushcfunction(L, lua_AudioPlay3D);    lua_setfield(L, -2, "Play3D");
        lua_pushcfunction(L, lua_AudioIsPlaying); lua_setfield(L, -2, "IsPlaying");
        lua_pushcfunction(L, lua_AudioMute);      lua_setfield(L, -2, "Mute");
        lua_setglobal(L, "Sound");

        // Upgrade Screen
        // Inventory Upgrade - Upgrades hotbar slots
        lua_register(L, "UpgradeInventorySlots", [](lua_State* L) -> int {
            int amount = (int)luaL_checkinteger(L, 1);
            auto& upgradeMgr = UpgradeManager::Instance();
            bool success = upgradeMgr.UpgradeInventorySlots(amount);

            if (success)
            {
                // Actually apply the upgrade to inventory
                auto& inv = Inventory::Instance();
                int currentLevel = inv.GetHotbarLevel();
                inv.SetHotbarLevel(currentLevel + 1);

                std::cout << "[Lua] Inventory upgraded! New level: " << (currentLevel + 1)
                    << ", Slots: " << inv.GetHotbarSlotCount() << std::endl;
            }

            lua_pushboolean(L, success);
            return 1;
            });

        // Get inventory upgrade cost
        lua_register(L, "GetInventoryUpgradeCost", [](lua_State* L) -> int {
            int cost = UpgradeManager::Instance().GetInventoryUpgradeCost();
            lua_pushinteger(L, cost);
            return 1;
            });

        // Get current hotbar slot count
        lua_register(L, "GetInventorySlotCount", [](lua_State* L) -> int {
            int slots = Inventory::Instance().GetHotbarSlotCount();
            lua_pushinteger(L, slots);
            return 1;
            });

        // Get current hotbar level
        lua_register(L, "GetInventoryLevel", [](lua_State* L) -> int {
            int level = Inventory::Instance().GetHotbarLevel();
            lua_pushinteger(L, level);
            return 1;
            });

        // Upgrade backpack
        lua_register(L, "UpgradeBackpack", [](lua_State* L) -> int {
            bool success = UpgradeManager::Instance().UpgradeBackpack();
            lua_pushboolean(L, success);
            return 1;
            });

        // Add backpack cost getter
        lua_register(L, "GetBackpackUpgradeCost", [](lua_State* L) -> int {
            int cost = UpgradeManager::Instance().GetBackpackUpgradeCost();
            lua_pushinteger(L, cost);
            return 1;
            });

        // Get backpack level
        lua_register(L, "GetBackpackLevel", [](lua_State* L) -> int {
            int level = Inventory::Instance().GetBackpackLevel();
            lua_pushinteger(L, level);
            return 1;
            });

        // Get backpack slot count
        lua_register(L, "GetBackpackSlotCount", [](lua_State* L) -> int {
            int slots = Inventory::Instance().GetBackpackSlotCount();
            lua_pushinteger(L, slots);
            return 1;
            });

        // Currency Upgrade
        lua_register(L, "GetCurrency", [](lua_State* L) -> int {
            int currency = GameSystem::GetCurrency();
            lua_pushinteger(L, currency);
            return 1;
            });

        lua_register(L, "SetCurrency", [](lua_State* L) -> int {
            int value = static_cast<int>(luaL_checkinteger(L, 1));
            GameSystem::SetCurrency(value);
            return 0;
            });

        lua_register(L, "AddCurrency", [](lua_State* L) -> int {
            int delta = static_cast<int>(luaL_checkinteger(L, 1));
            GameSystem::AddCurrency(delta);
            return 0;
            });

        // Currency Upgrade functions
        lua_register(L, "UpgradeCurrencyMultiplier", [](lua_State* L) -> int {
            float bonus = static_cast<float>(luaL_checknumber(L, 1));
            bool success = UpgradeManager::Instance().UpgradeCurrencyMultiplier(bonus);
            lua_pushboolean(L, success);
            return 1;
            });

        lua_register(L, "GetCurrencyMultiplier", [](lua_State* L) -> int {
            float multiplier = UpgradeManager::Instance().GetCurrencyMultiplier();
            lua_pushnumber(L, multiplier);
            return 1;
            });

        lua_register(L, "GetCurrencyUpgradeCost", [](lua_State* L) -> int {
            int cost = UpgradeManager::Instance().GetCurrencyUpgradeCost();
            lua_pushinteger(L, cost);
            return 1;
            });

        lua_register(L, "GetCurrencyLevel", [](lua_State* L) -> int {
            int level = UpgradeManager::Instance().GetCurrencyLevel();
            lua_pushinteger(L, level);
            return 1;
            });

        // =========================================================
        // DAY STATS BINDINGS
        // =========================================================

        lua_register(L, "AddCustomerServed", [](lua_State* L) -> int {
            GameSystem::AddData("dayCustomersServed", 1);
            return 0;
            });

        lua_register(L, "AddCustomerSpawned", [](lua_State* L) -> int {
            GameSystem::AddData("dayCustomersSpawned", 1);
            return 0;
            });

        lua_register(L, "AddCustomerFailed", [](lua_State* L) -> int {
            GameSystem::AddData("dayCustomersFailed", 1);
            return 0;
            });

        lua_register(L, "GetLastDayStats", [](lua_State* L) -> int {
            Framework::GameSystem::DayStats d = GameSystem::GetLastDayStats();
            lua_newtable(L);
            lua_pushinteger(L, d.dayIndex);           lua_setfield(L, -2, "dayIndex");
            lua_pushinteger(L, d.goldFromCustomers);  lua_setfield(L, -2, "goldFromCustomers");
            lua_pushinteger(L, d.timeBonusGold);      lua_setfield(L, -2, "timeBonusGold");
            lua_pushinteger(L, d.totalGold);          lua_setfield(L, -2, "totalGold");
            lua_pushinteger(L, d.customersServed);    lua_setfield(L, -2, "customersServed");
            lua_pushinteger(L, d.customersSpawned);   lua_setfield(L, -2, "customersSpawned");
            lua_pushnumber(L, d.timeRemainingX100 / 100.0); lua_setfield(L, -2, "timeRemaining");
            lua_pushinteger(L, d.stars);              lua_setfield(L, -2, "stars");
            return 1;
            });

        lua_register(L, "GetAllDayStats", [](lua_State* L) -> int {
            const auto& history = GameSystem::GetDayHistory();
            lua_newtable(L);
            for (int i = 0; i < (int)history.size(); ++i) {
                const Framework::GameSystem::DayStats& d = history[i];
                lua_newtable(L);
                lua_pushinteger(L, d.dayIndex);           lua_setfield(L, -2, "dayIndex");
                lua_pushinteger(L, d.goldFromCustomers);  lua_setfield(L, -2, "goldFromCustomers");
                lua_pushinteger(L, d.timeBonusGold);      lua_setfield(L, -2, "timeBonusGold");
                lua_pushinteger(L, d.totalGold);          lua_setfield(L, -2, "totalGold");
                lua_pushinteger(L, d.customersServed);    lua_setfield(L, -2, "customersServed");
                lua_pushinteger(L, d.customersSpawned);   lua_setfield(L, -2, "customersSpawned");
                lua_pushnumber(L, d.timeRemainingX100 / 100.0); lua_setfield(L, -2, "timeRemaining");
                lua_pushinteger(L, d.stars);              lua_setfield(L, -2, "stars");
                lua_rawseti(L, -2, i + 1);
            }
            return 1;
            });

        lua_register(L, "GetTotalStars", [](lua_State* L) -> int {
            const auto& history = GameSystem::GetDayHistory();
            int total = 0;
            for (const auto& d : history) total += d.stars;
            lua_pushinteger(L, total);
            return 1;
            });

        lua_register(L, "DeductPhaseTime", [](lua_State* L) -> int {
            float seconds = (float)lua_tonumber(L, 1);
            Entity timerEnt = ecsInterface.GetEntityByName("PhaseTimerText");
            if (ecsInterface.IsEntityValid(timerEnt) &&
                ecsInterface.HasComponent<TimerComponent>(timerEnt))
            {
                auto& timer = ecsInterface.GetComponent<TimerComponent>(timerEnt);
                timer.remainingTime = std::max(0.0f, timer.remainingTime - seconds);
            }
            return 0;
            });

        // Area Unlock Functions
        lua_register(L, "UnlockForest", [](lua_State* L) -> int {
            bool success = UpgradeManager::Instance().UnlockArea(GameArea::Forest);
            lua_pushboolean(L, success);
            return 1;
            });

        lua_register(L, "UnlockQuarry", [](lua_State* L) -> int {
            bool success = UpgradeManager::Instance().UnlockArea(GameArea::Quarry);
            lua_pushboolean(L, success);
            return 1;
            });

        lua_register(L, "IsForestUnlocked", [](lua_State* L) -> int {
            bool unlocked = UpgradeManager::Instance().IsAreaUnlocked(GameArea::Forest);
            lua_pushboolean(L, unlocked);
            return 1;
            });

        lua_register(L, "IsQuarryUnlocked", [](lua_State* L) -> int {
            bool unlocked = SceneInterface::GetCurrentDay() >= 3; // whatever day you want
            lua_pushboolean(L, unlocked);
            return 1;
            });

        lua_register(L, "GetForestUnlockCost", [](lua_State* L) -> int {
            int cost = UpgradeManager::Instance().GetAreaUnlockCost(GameArea::Forest);
            lua_pushinteger(L, cost);
            return 1;
            });

        lua_register(L, "GetQuarryUnlockCost", [](lua_State* L) -> int {
            int cost = UpgradeManager::Instance().GetAreaUnlockCost(GameArea::Quarry);
            lua_pushinteger(L, cost);
            return 1;
            });

        lua_register(L, "UpgradeCauldron", [](lua_State* L) -> int {
            bool success = UpgradeManager::Instance().UpgradeCauldron();
            lua_pushboolean(L, success);
            return 1;
            });

        lua_register(L, "IsCauldronUnlocked", [](lua_State* L) -> int {
            bool unlocked = UpgradeManager::Instance().IsCauldronUnlocked();
            lua_pushboolean(L, unlocked);
            return 1;
            });

        lua_register(L, "GetCauldronUpgradeCost", [](lua_State* L) -> int {
            int cost = UpgradeManager::Instance().GetCauldronUpgradeCost();
            lua_pushinteger(L, cost);
            return 1;
            });

        std::cout << "[ScriptingSystem] Initialized (Lua VM ready)\n";
    }

    void ScriptingSystem::Update(float dt)
    {
        // CREATE A COPY of the entity set to iterate over
        std::vector<Entity> entitiesToProcess;
        entitiesToProcess.reserve(mEntities.size());
        for (Entity e : mEntities) {
            entitiesToProcess.push_back(e);
        }

        // NOW iterate over the COPY, not the original set
        for (Entity e : entitiesToProcess) {
            // Add safety check: entity might have been destroyed by a previous script
            if (!ecsInterface.IsEntityValid(e)) {
                continue;
            }

            if (!ecsInterface.HasComponent<ScriptProperties>(e)) continue;
            auto& comp = ecsInterface.GetComponent<ScriptProperties>(e);

            for (auto& sc : comp.scripts) {
                if (sc.scriptPath.empty()) continue;

                // ----- AUTO HOT RELOAD POLLING -----
                if (sc.hotReload) {
                    std::error_code ec;
                    auto ft = std::filesystem::last_write_time(sc.scriptPath, ec);
                    if (ec) {
                        static std::unordered_set<std::string> alreadyLogged;
                        if (!alreadyLogged.count(sc.scriptPath)) {
                            std::cerr << "[HotReload] last_write_time failed for "
                                << sc.scriptPath << ": " << ec.message() << "\n";
                            alreadyLogged.insert(sc.scriptPath);
                        }
                        sc.hotReload = false;
                    }
                    else {
                        auto nowTicks = FileTimeToTicks(ft);
                        if (nowTicks != sc.lastWriteTicks) {
                            std::cout << "[HotReload] " << sc.scriptPath << "\n";
                            if (LoadScriptTable(sc)) {
                                lua_rawgeti(L, LUA_REGISTRYINDEX, sc.tableRef);
                                lua_pushinteger(L, static_cast<lua_Integer>(e));
                                lua_setfield(L, -2, "entity");
                                lua_pop(L, 1);
                                CallHook(sc.tableRef, "on_init");
                                sc.initialized = true;
                            }
                            sc.lastWriteTicks = nowTicks;
                        }
                    }
                }

                if (sc.tableRef == LUA_NOREF || sc.tableRef == -1)
                {
                    if (!LoadScriptTable(sc)) continue;
                    lua_rawgeti(L, LUA_REGISTRYINDEX, sc.tableRef);
                    lua_pushinteger(L, static_cast<lua_Integer>(e));
                    lua_setfield(L, -2, "entity");
                    lua_pop(L, 1);
                    sc.initialized = false;
                }

                if (!sc.initialized) {
                    CallHook(sc.tableRef, "on_init");
                    sc.initialized = true;
                }

                CallHook(sc.tableRef, "on_update", dt);
            }
        }
    }

    void ScriptingSystem::SetProjectRoot(const std::string& root) {
        std::error_code ec;
        auto canonical = std::filesystem::canonical(root, ec);
        if (!ec) {
            s_ProjectRoot = canonical.generic_string();
            std::cout << "[ScriptingSystem] Project root: " << s_ProjectRoot << "\n";
        }
    }

    std::string ScriptingSystem::GetProjectRoot() { return s_ProjectRoot; }

    std::string ScriptingSystem::GetName() { return "ScriptingSystem"; }

    bool ScriptingSystem::LoadScriptTable(ScriptEntry& sc)
    {
        if (sc.tableRef != LUA_NOREF) {
            luaL_unref(L, LUA_REGISTRYINDEX, sc.tableRef);
            sc.tableRef = LUA_NOREF;
        }

        // BUILD ABSOLUTE PATH FROM PROJECT ROOT
        std::filesystem::path scriptPath;
        if (std::filesystem::path(sc.scriptPath).is_absolute()) {
            scriptPath = sc.scriptPath;
        }
        else {
            scriptPath = std::filesystem::path(s_ProjectRoot) / sc.scriptPath;
        }

        std::error_code ec;
        auto canonicalPath = std::filesystem::weakly_canonical(scriptPath, ec);

        if (ec || !std::filesystem::exists(canonicalPath)) {
            std::cerr << "[Lua] script file not found: " << sc.scriptPath
                << " (resolved to: " << scriptPath << ")\n";
            return false;
        }

        std::string absoluteScriptPath = canonicalPath.generic_string();

        // USE ABSOLUTE PATH
        if (luaL_dofile(L, absoluteScriptPath.c_str()) != LUA_OK) {
            std::cerr << "[Lua] load error in " << sc.scriptPath << ": " << lua_tostring(L, -1) << "\n";
            lua_pop(L, 1);
            return false;
        }

        if (!lua_istable(L, -1)) {
            std::cerr << "[Lua] script must `return` a table: " << sc.scriptPath << "\n";
            lua_pop(L, 1);
            return false;
        }

        sc.tableRef = luaL_ref(L, LUA_REGISTRYINDEX);
        sc.initialized = false;
        return true;
    }

    bool ScriptingSystem::CallHook(int tableRef, const char* fn, float arg)
    {
        if (tableRef <= 0 || fn == nullptr)
            return false;

        // Push the instance table
        lua_rawgeti(L, LUA_REGISTRYINDEX, tableRef);  // [tbl]
        lua_getfield(L, -1, fn);                      // [tbl, fn]

        if (!lua_isfunction(L, -1)) {
            lua_pop(L, 2);                            // pop fn, tbl
            return false;
        }

        // self (the table) as first arg
        lua_pushvalue(L, -2);                         // [tbl, fn, tbl]
        int nargs = 1;

        if (std::strcmp(fn, "on_update") == 0) {
            lua_pushnumber(L, arg);                  // [tbl, fn, tbl, dt]
            nargs = 2;
        }

        if (lua_pcall(L, nargs, 0, 0) != LUA_OK) {
            const char* err = lua_tostring(L, -1);
            std::cerr << "[Lua Error] in " << fn << ": " << (err ? err : "(unknown)") << "\n";
            lua_pop(L, 1); // pop error
            lua_pop(L, 1); // pop tbl
            return false;
        }

        lua_pop(L, 1); // pop tbl
        return true;
    }

    Entity ScriptingSystem::CreateScriptEntity(const std::string& path) {
        Entity e = ecsInterface.CreateEntity();

        ScriptProperties comp;
        ScriptEntry inst;
        inst.scriptPath = path;
        comp.scripts.push_back(inst);

        ecsInterface.AddComponent<ScriptProperties>(e, comp);
        ecsInterface.AddComponent<TransformComponent>(e, TransformComponent{});
        ecsInterface.AddComponent<HierarchyComponent>(e, HierarchyComponent{});
        return e;
    }

    void ScriptingSystem::HotReloadScript(const std::string& changedPath)
    {
        namespace fs = std::filesystem;
        std::error_code ec;

        fs::path changedAbs = fs::weakly_canonical(changedPath, ec);
        std::string changedNorm = (ec ? fs::path(changedPath) : changedAbs)
            .lexically_normal().generic_string();
        std::string changedFile = fs::path(changedNorm).filename().string();

        std::cout << "[HotReload] request for: " << changedNorm << "\n";

        bool any = false;

        for (auto e : mEntities)
        {
            if (!ecsInterface.HasComponent<ScriptProperties>(e)) continue;
            auto& comp = ecsInterface.GetComponent<ScriptProperties>(e);

            for (auto& sc : comp.scripts) {
                if (sc.scriptPath.empty()) continue;

                ec.clear();
                fs::path scAbs = fs::weakly_canonical(sc.scriptPath, ec);
                std::string scNorm = (ec ? fs::path(sc.scriptPath) : scAbs)
                    .lexically_normal().generic_string();
                std::string scFile = fs::path(scNorm).filename().string();

                bool match = (scNorm == changedNorm) || (scFile == changedFile);

                if (match)
                {
                    std::cout << "  -> reloading entity " << e
                        << " (" << scNorm << ")\n";
                    if (LoadScriptTable(sc))
                    {
                        lua_rawgeti(L, LUA_REGISTRYINDEX, sc.tableRef);
                        lua_pushinteger(L, static_cast<lua_Integer>(e));
                        lua_setfield(L, -2, "entity");
                        lua_pop(L, 1);

                        sc.initialized = false;
                        CallHook(sc.tableRef, "on_init");
                        any = true;

                        std::error_code ecc;
                        auto ft = std::filesystem::last_write_time(sc.scriptPath, ecc);
                        if (!ecc)
                        {
                            sc.lastWriteTicks = FileTimeToTicks(ft);
                        }
                    }
                    else
                    {
                        std::cerr << "  !! reload failed for " << scNorm << "\n";
                    }
                }
            }
        }
    }
} // namespace Framework