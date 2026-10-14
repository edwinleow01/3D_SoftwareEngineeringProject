-- footstep.lua
local M = {}

local stepTimer = 0
local stepInterval = 0.5

local sheepTimer = 0
local sheepInterval = math.random(15, 25) -- seconds between sheep sounds

local sheepEntities = {}

-- Footstep
local indoorEvent = "event:/Footstep"
local outdoorEvent = "event:/FootstepBackyard"
local quarryFootstepEvent = "event:/FootstepQuarry"

-- BGM 
local level1BGM = "event:/Level1BGM"
local quarryBGM = "event:/QuarryBGM"
local shopBGM = "event:/ShopBGM"

-- Ambient related sounds 
local sheepEvent = "event:/SheepAmbient"
local quarryEvent = "event:/QuarryAmbient"

--Track state
local wasInBackyard = false
local backyardEntity = nil
local backyardCopyEntity = nil
local frogEntity = nil 
local quarryPlaying = false

local wasInQuarry = false
local wasInShop = false

local firstPlayFrame = true

-- Forest/Backyard bounds (measured in-game)
local backyardMin = { 
    x = -296, 
    y = 300, 
    z = 533 
}

local backyardMax = { 
    x = -120, 
    y = 330, 
    z = 726 
}

-- Quarry bounds 
local quarryMin = {
    x = -350,
    y = 240,
    z = -300
}

local quarryMax = { 
    x = -80,
    y = 400,
    z = 0
}

-- Shop bounds 
local shopMin = { 
    x = -52, 
    y = -5, 
    z = -1 
}

local shopMax = { 
    x = -14, 
    y = 15, 
    z = 21 
}

function M:on_init()
    print("[Footsteps] Script initialized for entity " .. tostring(self.entity))
    print(string.format("[Footsteps] Backyard bounds: X(%.2f to %.2f), Y(%.2f to %.2f), Z(%.2f to %.2f)", 
        backyardMin.x, backyardMax.x, backyardMin.y, backyardMax.y, backyardMin.z, backyardMax.z))

    -- Find both backyard entities for volume control
    backyardEntity = FindEntityByName("Backyard")
    backyardCopyEntity = FindEntityByName("Backyard (Copy)")

    if backyardEntity then
        print("[Footsteps] Found Backyard entity: " .. backyardEntity)
    else
        print("[Footsteps] WARNING: Could not find Backyard entity!")
    end 

    if backyardCopyEntity then
        print("[Footsteps] Found Backyard (Copy) entity: " .. backyardCopyEntity)
    else
        print("[Footsteps] WARNING: Could not find Backyard (Copy) entity!")
    end

    -- Find the frog entity for volume control
    frogEntity = FindEntityByName("Frog")
    if frogEntity then
        print("[Footsteps] Found Frog entity: " .. frogEntity)
    else
        print("[Footsteps] WARNING: Could not find Frog entity!")
    end

    -- Find sheep entities
    local animalNames = {"Animal_1", "Animal_2", "Animal_3", "Animal_4"}
    for _, name in ipairs(animalNames) do
        local ent = FindEntityByName(name)
        if ent then
            table.insert(sheepEntities, ent)
            print("[Footsteps] Found sheep: " .. name .. " (entity " .. ent .. ")")
        end
    end

end

-- Zone Check for Forest 
function M:IsInBackyard()
    local px, py, pz = GetPosition(self.entity)
    
    local inBackyard = px >= backyardMin.x and px <= backyardMax.x and
                       py >= backyardMin.y and py <= backyardMax.y and
                       pz >= backyardMin.z and pz <= backyardMax.z
    
    return inBackyard
end

-- Zone Check for Quarry
function M:IsInQuarry()
    local px, py, pz = GetPosition(self.entity)
    return px >= quarryMin.x and px <= quarryMax.x and
           py >= quarryMin.y and py <= quarryMax.y and
           pz >= quarryMin.z and pz <= quarryMax.z
end

-- Zone Check for Shop
function M:IsInShop()
    local px, py, pz = GetPosition(self.entity)
    return px >= shopMin.x and px <= shopMax.x and
           py >= shopMin.y and py <= shopMax.y and
           pz >= shopMin.z and pz <= shopMax.z
end


function M:on_update(dt)

    -- Only run audio logic during actual gameplay (no UI overlays)
    -- Check for all states that mean "not in free gameplay"
    if HasEngineState("Menu") 
        or HasEngineState("Intro") 
        or HasEngineState("Cutscene")
        or HasEngineState("MapSelection")
        or HasEngineState("Day2")
        or HasEngineState("Day3")
        or HasEngineState("Day4")
        or HasEngineState("Day5")
        or HasEngineState("Win")
        or HasEngineState("Lose")
        or HasEngineState("Credit")
        or HasEngineState("DayStats")
        or HasEngineState("Upgrade")
        or HasEngineState("Paused")
    then 
        firstPlayFrame = true
        return 
    end

    -- On first frame back in gameplay, reset zone state and play correct BGM
    if firstPlayFrame then
        firstPlayFrame = false
        wasInQuarry = false
        wasInShop = false
        wasInBackyard = false
        Sound.Stop(shopBGM)
        Sound.Stop(quarryBGM)
        Sound.Stop(level1BGM)

        -- Immediately play the correct BGM for current zone
        if self:IsInQuarry() then
            Sound.Play(quarryBGM)
            wasInQuarry = true
        elseif self:IsInShop() then
            Sound.Play(shopBGM)
            wasInShop = true
        else
            Sound.Play(level1BGM)
        end
    end

    local isInBackyard = self:IsInBackyard()

    -- Handle ambient occlusion (volume control based on location)
    -- Must mute BOTH Backyard and Backyard (Copy) entities
    if isInBackyard then
        if backyardEntity then SetEntityVolume(backyardEntity, 1.0) end
        if backyardCopyEntity then SetEntityVolume(backyardCopyEntity, 1.0) end
    else
        if backyardEntity then SetEntityVolume(backyardEntity, 0.0) end
        if backyardCopyEntity then SetEntityVolume(backyardCopyEntity, 0.0) end
    end

    -- Handle frog occlusion (opposite of backyard)
    if frogEntity then
        if isInBackyard then
            -- Muted when in backyard (outdoor)
            SetEntityVolume(frogEntity, 0.0)
        else
            -- Full volume when indoors
            SetEntityVolume(frogEntity, 1.0)
        end
    end

    -- Handle quarry ambient sound
    local isInQuarry = self:IsInQuarry()
    if isInQuarry and not quarryPlaying then
        Sound.Play(quarryEvent)
        quarryPlaying = true
        print("[Footsteps] Entered quarry - playing QuarryAmbient")
    elseif not isInQuarry and quarryPlaying then
        Sound.Stop(quarryEvent)
        quarryPlaying = false
        print("[Footsteps] Left quarry - stopped QuarryAmbient")
    end

    -- Log zone transitions for debugging 
    if isInBackyard and not wasInBackyard then
        print("[Footsteps] Entered backyard - ambient at full volume, frog muted")
    elseif not isInBackyard and wasInBackyard then
        print("[Footsteps] Left backyard - ambient muffled, frog audible")
    end 

    wasInBackyard = isInBackyard

    -- Handle BGM switching (unified)
    local isInShop = self:IsInShop()
    local currentZone = "overworld"
    if isInQuarry then currentZone = "quarry"
    elseif isInShop then currentZone = "shop" end

    local previousZone = "overworld"
    if wasInQuarry then previousZone = "quarry"
    elseif wasInShop then previousZone = "shop" end

    if currentZone ~= previousZone then
        -- Stop the old BGM
        if previousZone == "quarry" then Sound.Stop(quarryBGM)
        elseif previousZone == "shop" then Sound.Stop(shopBGM)
        else Sound.Stop(level1BGM) end

        -- Play the new BGM
        if currentZone == "quarry" then Sound.Play(quarryBGM)
        elseif currentZone == "shop" then Sound.Play(shopBGM)
        else Sound.Play(level1BGM) end

        print("[Footsteps] BGM: " .. previousZone .. " -> " .. currentZone)
    end

    wasInQuarry = isInQuarry
    wasInShop = isInShop

    -- Sheep ambient sounds
    if #sheepEntities > 0 then
        sheepTimer = sheepTimer + dt

        if sheepTimer >= sheepInterval then
            -- Pick a random sheep
            local sheepEnt = sheepEntities[math.random(1, #sheepEntities)]

            -- Play sheep sound (FMOD randomizes internally)
            local sx, sy, sz = GetPosition(sheepEnt)
            Sound.Play3D(sheepEvent, sx, sy, sz)

            print("[Footsteps] Sheep bleat from entity " .. sheepEnt)

            -- Reset timer
            sheepTimer = 0
            sheepInterval = math.random(15, 25)
        end
    end

    -- Footstep logic 
    -- Check if player is moving
    local isMoving = false
    
    if IsKeyDown(87) or IsKeyDown(83) or IsKeyDown(65) or IsKeyDown(68) then
        isMoving = true
    end
    
    if isMoving then
        stepTimer = stepTimer + dt
        if stepTimer >= stepInterval then
            -- Check zone and play appropriate footstep
            local eventToPlay = indoorEvent

            if isInBackyard then eventToPlay = outdoorEvent
            elseif isInQuarry then eventToPlay = quarryFootstepEvent 
            
            end
            
            Sound.Play(eventToPlay)
            stepTimer = 0
            
            -- Debug output (optional - remove later)
            print("[Footsteps] Playing: " .. eventToPlay .. " | In backyard: " .. tostring(isInBackyard))
        end
    else
        stepTimer = stepInterval * 0.5
    end
end

return M