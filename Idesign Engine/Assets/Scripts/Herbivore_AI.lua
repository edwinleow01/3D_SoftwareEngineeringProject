--  Animal Wander AI
local M = {}

M.wanderRange = 10.0
M.waitMin = 2.0
M.waitMax = 5.0
M.arrivalDist = 1.5

-- 10 seconds converted to milliseconds for SetData
local REGROWTH_TIME_MS = 180000

-- =========================================================
-- HELPERS: per-entity state via SetData/GetData
-- =========================================================
local function GetState(entity, key)
    local val = GetData(key .. "_" .. tostring(entity))
    return val or 0
end

local function SetState(entity, key, value)
    SetData(key .. "_" .. tostring(entity), value)
end

-- =========================================================
-- ON INIT
-- =========================================================
function M:on_init()
    self.timer = math.random() * (M.waitMax - M.waitMin) + M.waitMin
    self.targetPos = nil
    self.waiting = true
    
    -- Initialize the sheep's wool state in the C++ backend
    SetState(self.entity, "IsSheared", 0)
    SetState(self.entity, "RegrowthTimer", 0)
end

-- =========================================================
-- ON UPDATE
-- =========================================================
function M:on_update(dt)
    -- ==========================================
    -- WANDER LOGIC
    -- ==========================================
    if self.waiting then
        self.timer = self.timer - dt
        if self.timer <= 0 then
            local x, y, z = GetPosition(self.entity)
            local newX = x + (math.random() - 0.5) * 2 * M.wanderRange
            local newZ = z + (math.random() - 0.5) * 2 * M.wanderRange
            
            PathfindToPos(self.entity, newX, y, newZ)
            
            self.targetPos = {x = newX, y = y, z = newZ}
            self.waiting = false
        end
    else
        local x, y, z = GetPosition(self.entity)
        local dx = self.targetPos.x - x
        local dz = self.targetPos.z - z
        local dist = math.sqrt(dx*dx + dz*dz)
        
        if dist <= M.arrivalDist then
            self.waiting = true
            self.timer = math.random() * (M.waitMax - M.waitMin) + M.waitMin
        end
    end

    -- ==========================================
    -- WOOL REGROWTH LOGIC
    -- ==========================================
    if GetState(self.entity, "IsSheared") == 1 then
        
        -- Subtract ms from the timer
        local current_timer = GetState(self.entity, "RegrowthTimer")
        current_timer = current_timer - math.floor(dt * 1000)
        
        if current_timer <= 0 then
            -- Wool has fully grown back!
            SetState(self.entity, "IsSheared", 0)
            print("[Herbivore_AI] Wool regrew on sheep " .. tostring(self.entity) .. "!")
            
            SetParticleEmitterActive(self.entity, true)

            -- Swap BACK to the fluffy sheep model
            if SetMesh then SetMesh(self.entity, "Assets/Objects/SheepWithWool.obj") end
            if SetTexture then SetTexture(self.entity, "Assets/Textures/SheepWithWool.png") end
            
            if SetInteractionText then
                SetInteractionText(self.entity, "Collect Wool")
            end
        else
            SetState(self.entity, "RegrowthTimer", current_timer)
        end
    end
end

-- =========================================================
-- ON INTERACT (Called by EventReceiverComponent)
-- =========================================================
function M.OnInteract(senderID, targetID, actionType)
    
    -- targetID is guaranteed to be THIS specific sheep, 
    -- so we don't need 'self' or any extra checks!
    
    if GetState(targetID, "IsSheared") == 0 then

        -- Check inventory before shearing
        if IsInventoryFull() then
            local log = SpawnPrefab("CauldronLog_Prefab")
            if log and log ~= -1 then
                SetTextComponent(log, "text", "Inventory Full! Could not pick up Wool")
                Sound.Play("event:/InvalidRecipe") 
            end
            return
        end

        print("[Herbivore_AI] Sheep " .. tostring(targetID) .. " sheared!")

        -- 1. Trigger the native C++ Zip Pickup!
        local px, py, pz = GetPosition(targetID)
        if TriggerZipPickup then
            TriggerZipPickup("wool", px, py, pz)
        else
            if AddToInventory then AddToInventory("wool") end
        end

        -- 2. Swap using clean, relative paths
        if SetMesh then SetMesh(targetID, "Assets/Objects/SheepWithoutWool.obj") end
        if SetTexture then SetTexture(targetID, "Assets/Textures/SheepWithoutWool.png") end

        SetParticleEmitterActive(targetID, false)

        -- 3. Update the C++ state so it can't be sheared again immediately
        SetState(targetID, "IsSheared", 1)
        SetState(targetID, "RegrowthTimer", REGROWTH_TIME_MS)
        
        if SetInteractionText then
            SetInteractionText(targetID, "Wool Regrowing...")
        end
    else
        print("[Herbivore_AI] Sheep " .. tostring(targetID) .. " has no wool right now.")
    end
end

return M