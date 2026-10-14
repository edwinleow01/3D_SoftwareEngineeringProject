local M = {}

-- --- 1. CONFIGURATION ---
M.knife_offset_right = 0.4       
M.knife_offset_up = -0.4         
M.knife_offset_forward = 0.5    

M.knife_rot_held_x = 0
M.knife_rot_held_y = -90.0 
M.knife_rot_held_z = 0.0

-- --- 2. STATE ---
local player_entity = nil
local knife_entity = nil
local is_in_inventory = false
local last_phase = nil

-- Storage for the original home position
local start_x, start_y, start_z = 0, 0, 0
local start_rx, start_ry, start_rz = 0, 0, 0

function M:on_init()
    player_entity = FindEntityByName("Player")
    knife_entity = FindEntityByName("Knife")

    -- 1. SAVE STARTING TRANSFORMS
    if knife_entity then
        start_x, start_y, start_z = GetPosition(knife_entity)
        start_rx, start_ry, start_rz = GetRotation(knife_entity)
        -- Set initial text
        SetInteractionText(self.entity, "Pick up Knife")
    end
end

-- This is called by C++ PlayerInteractSystem when you press 'E'
function M.KnifePickUp(entityID)
    
    -- === LOGIC BRANCH: PUT DOWN vs PICK UP ===
    
    if is_in_inventory then
        -- [PUT DOWN LOGIC]
        
        -- Optional: Ensure we are actually holding the knife to put it back
        local heldItem = GetSelectedItemName()
        if heldItem == "knife" then
            
            -- 1. Remove from Inventory (Consumes the currently held item)
            ConsumeSelectedItem()

            -- 2. Teleport back to home position
            -- (We use the variables we saved in on_init)
            SetPosition(knife_entity, start_x, start_y, start_z)
            SetRotation(knife_entity, start_rx, start_ry, start_rz)

            -- 3. Reset State
            is_in_inventory = false
            SetInteractionText(entityID, "Pick up Knife")
            
            print("[Knife] Put back on table.")
        else
            print("[Knife] You must be holding the knife to put it back!")
        end

    else
        -- [PICK UP LOGIC]
        
        local myName = GetItemName(entityID)
        local slot = AddToInventory(myName)

        if slot >= 0 then
            print("[Knife] Picked up! Moved to slot " .. slot)
            
            -- Mark as in inventory
            is_in_inventory = true
            
            -- Change text so the next interaction is "Put down"
            SetInteractionText(entityID, "Put down Knife")
        else
            print("[Knife] Inventory Full")
        end
    end

    return true
end

function M:on_update(dt)

    -- Phase reset check
    local current_phase = GetPhase()
    if current_phase ~= last_phase then
        if current_phase == 1 then  -- Day phase, nid to match enum class GamePhase
            self:ResetKnife()
        end
        last_phase = current_phase
    end

    -- 1. If we haven't picked it up yet, do nothing (it sits on the table)
    if is_in_inventory == false then

        return 
    end
    -- 2. If we HAVE picked it up, check if we are holding it
    local heldItemName = GetSelectedItemName() --

    if heldItemName == "knife" then
        -- WE ARE HOLDING IT: Move to Hand
        self:UpdateKnifePosition()
    else
        -- WE ARE NOT HOLDING IT: Hide it under the map
        -- (This is the "Teleport/Hide" method)
        SetPosition(knife_entity, 0, -1000, 0)
    end
end

function M:UpdateKnifePosition()
    if not player_entity then 
        player_entity = FindEntityByName("Player") 
        return
    end

    local px, py, pz = GetPosition(player_entity)
    local camYaw = GetCameraYaw() --
    local rad = math.rad(camYaw)

    -- Math to find the position relative to the camera rotation
    local fwdX = math.cos(rad)
    local fwdZ = math.sin(rad)
    local rightX = -math.sin(rad)
    local rightZ = math.cos(rad)

    local kx = px + (rightX * M.knife_offset_right) + (fwdX * M.knife_offset_forward)
    local ky = py + M.knife_offset_up 
    local kz = pz + (rightZ * M.knife_offset_right) + (fwdZ * M.knife_offset_forward)

    SetPosition(knife_entity, kx, ky, kz)
    SetRotation(knife_entity, M.knife_rot_held_x, -camYaw + M.knife_rot_held_y, M.knife_rot_held_z)
end

function M:ResetKnife()
    if is_in_inventory then
        RemoveItemByName("knife")
        SetPosition(knife_entity, start_x, start_y, start_z)
        SetRotation(knife_entity, start_rx, start_ry, start_rz)
        is_in_inventory = false
        SetInteractionText(self.entity, "Pick up Knife")
        print("[Knife] Reset to spawn position for Day phase.")
    end
end

return M