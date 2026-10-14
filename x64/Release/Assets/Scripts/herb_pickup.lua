local M = {}

function M.OnPickup(entityID)
    -- 1. Get the name stored in the component
    local myName = GetItemName(entityID) 
    
    -- [DEBUG] Check exactly what this string is
    print("Lua Debug: Item Name is '" .. tostring(myName) .. "'")

    if IsInventoryFull() then
        local log = SpawnPrefab("CauldronLog_Prefab")
        if log and log ~= -1 then
            SetTextComponent(log, "text", "Inventory Full!")
            Sound.Play("event:/InvalidRecipe") 
        end
        return true
    end

    local slot = AddToInventory(myName)

    -- 3. Check result
    if slot >= 0 then
        
        print("Lua: Success! Added to slot " .. slot)

        -- [NEW] Play Pickup Sound
        if Sound and Sound.Play then
            Sound.Play("event:/PickingUpHerb")
        end

        if IsItemDestroyOnPick(entityID) then
            DestroyEntity(entityID)
        end
    elseif slot == -1 then
        -- This assumes your C++ returns -1 for full, but let's verify
        print("Lua Error: AddToInventory returned -1 (Inventory Full OR Item Not Found)")
    end

    return true
end

return M