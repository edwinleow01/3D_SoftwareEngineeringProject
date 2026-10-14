print("!!! LUA SCRIPT LOADING: herb_spawner.lua !!!") -- Top of file
SubscribeToEvent("RequestHerbSpawn", "RespawnHerbs", FindEntityByName("itemSpawner"))
local M = {}

function M.RespawnHerbs()
    print("EVENT RECEIVED: Respawning objects...")
    
    local herbs = {
        "Herb1", "Herb2", "Herb3", "Herb4", "ForestHerb", "ForestHerb2", 
        "ForestHerb3", "ForestHerb4", "ForestHerb5", "ForestHerb6", 
        "ForestHerb7", "Pear1", "Pear2", "Pear3", "Pear4", "Pear5", 
        "Pear6", "Pear7", "copperOre1", "copperOre2", "copperOre3", 
        "copperOre4", "copperOre5", "copperOre6", "copperOre7", 
        "ironOre1", "ironOre2", "ironOre3", "ironOre4", "ironOre5", "ironOre6", "ironOre7", "ironOre8"
    }

    for _, name in ipairs(herbs) do
        local id = FindEntityByName(name)
        -- Log this to see if FindEntityByName is actually working
        print("Checking: " .. name .. " | Found ID: " .. tostring(id)) 
        
        if not id or id == 0 or not IsEntityValid(id) then
            local prefabName = name .. "_Prefab"
            print("Spawning: " .. prefabName)
            SpawnPrefab(prefabName)
        end
    end
end

function M:on_init()
    -- Ensure your C++ SubscribeToEvent is looking for M.RespawnHerbs
    
end

return M