SubscribeToEvent("ResetWorkstations4", "ChoppingBoard_ResetStation", FindEntityByName("Cutting Board"))

local M = {}

-- --- CONFIGURATION ---
M.chop_button = 0                
M.interaction_range = 2.5
M.max_health = 5         

-- [CAMERA FLIGHT SETTINGS]
M.FLIGHT_DURATION = 0.8
M.FIXED_OFFSET_X = -0.5
M.FIXED_OFFSET_Y = 2.5
M.FIXED_OFFSET_Z = 3.0
M.FIXED_TARGET_PITCH = -50.0 
M.FIXED_TARGET_YAW = 270.0

-- Minigame Settings
M.marker_speed_base = 500.0     
M.marker_speed_increase = 150.0 
M.marker_limit = 540.0          
M.center_x = 960         
M.center_y = 340         

-- Collision Settings
M.zone_collision_width = 128 
M.zone_safe_margin = 460    

-- Herb Settings
M.place_offset_y = 0.1
M.place_offset_z = 0.3           
M.initial_item_name = "Herb"

-- Asset Paths
M.mesh_raw    = "Assets/Objects/Herb.obj"
M.tex_raw     = "Assets/Textures/Herb.png"
M.mesh_stage1 = "Assets/Objects/ChoppedHerb1.obj"
M.tex_stage1  = "Assets/Textures/ChoppedHerb1.png"
M.mesh_stage2 = "Assets/Objects/chopped_herb.obj"
M.tex_stage2  = "Assets/Textures/ChoppedHerb2.png"

-- Knife Settings
M.knife_name = "Knife"
M.knife_animation_name = "Armature|KnifeAction" 

M.ui_prefabs = {
    "UI_SkillBar_BG_Prefab",
    "UI_SkillBar_Zone_Prefab",
    "UI_SkillBar_Marker_Prefab"
}

-- --- INTERNAL STATE ---
local player_entity = nil
local current_herb_entity = nil
local knife_entity = nil
local current_health = 0
local click_was_down = false
local my_entity_id = nil

-- Camera Flight State
local is_camera_flying = false
local camera_flight_t = 0.0
local start_cam_pos = {x=0, y=0, z=0}
local start_cam_rot = {pitch=0, yaw=0}
local target_cam_pos = {x=0, y=0, z=0}
local target_cam_rot = {pitch=0, yaw=0}

-- UI State
local spawned_ui_ids = {}
local ui_active = false
local active_marker_id = nil 
local active_zone_id = nil   

-- Movement & Game State
local marker_pos_x = 0.0    
local marker_dir = 1        
local zone_pos_x = 0.0 
local current_speed = 500.0 
local last_phase = -1

-- =========================================================
-- ON INIT (SINGLE DEFINITION)
-- =========================================================
function M:on_init(arg1, arg2)
    -- BULLETPROOF ID GRABBER
    if type(arg1) == "table" and arg1.entity then my_entity_id = arg1.entity
    elseif type(arg1) == "number" then my_entity_id = arg1
    elseif type(arg2) == "number" then my_entity_id = arg2
    else my_entity_id = FindEntityByName("Chopping Board") end

    print("[ChoppingBoard] Initialized. ID: " .. tostring(my_entity_id))

    player_entity = FindEntityByName("Player")
    knife_entity = FindEntityByName(M.knife_name) -- This finds the Knife entity
    math.randomseed(os.time()) 
    
    if my_entity_id then
        -- Updated to match the function name below
        SubscribeToEvent("ResetWorkstations", "ChoppingBoard_ResetStation", my_entity_id)
    end
end

function M.OnInteract(entityID)
    if current_herb_entity ~= nil then return end

    local itemName = GetSelectedItemName()
    if itemName and itemName == "unchopped_herb" then
        M:StartChoppingSession(itemName)
        ConsumeSelectedItem()
        Sound.Play("event:/PlaceDownItem")
        SetColliderPromptText(entityID, "Pick up a knife and chop!")
    end
end

function M:on_update(dt)
    if not player_entity then player_entity = FindEntityByName("Player") end
    
    -- Phase checking to auto-reset on new day
    if GetPhase then
        local current_phase = GetPhase()
        if current_phase ~= last_phase then
            if current_phase == 1 then 
                ChoppingBoard_ResetStation()
            end
            last_phase = current_phase
        end
    end

    -- --- 0. MANUAL CAMERA FLIGHT ---
    if is_camera_flying then
        camera_flight_t = camera_flight_t + dt
        local t = math.min(camera_flight_t / self.FLIGHT_DURATION, 1.0)
        local smooth_t = t * t * (3 - 2 * t)

        local delta_yaw = target_cam_rot.yaw - start_cam_rot.yaw
        if delta_yaw > 180 then delta_yaw = delta_yaw - 360
        elseif delta_yaw < -180 then delta_yaw = delta_yaw + 360 end

        local cur_x = start_cam_pos.x + (target_cam_pos.x - start_cam_pos.x) * smooth_t
        local cur_y = start_cam_pos.y + (target_cam_pos.y - start_cam_pos.y) * smooth_t
        local cur_z = start_cam_pos.z + (target_cam_pos.z - start_cam_pos.z) * smooth_t
        
        local cur_p = start_cam_rot.pitch + (target_cam_rot.pitch - start_cam_rot.pitch) * smooth_t
        local cur_y_rot = start_cam_rot.yaw + (delta_yaw * smooth_t)

        TeleportPlayer(cur_x, cur_y, cur_z, 0, cur_y_rot, 0)
        SetCameraRot(cur_p, cur_y_rot)

        if t >= 1.0 then is_camera_flying = false end
        return 
    end

    -- --- 1. MINIGAME UI MOVEMENT ---
    if ui_active and active_marker_id ~= nil then
        marker_pos_x = marker_pos_x + (current_speed * marker_dir * dt)

        if marker_pos_x > M.marker_limit then
            marker_pos_x = M.marker_limit
            marker_dir = -1 
        elseif marker_pos_x < -M.marker_limit then
            marker_pos_x = -M.marker_limit
            marker_dir = 1  
        end

        SetPosition(active_marker_id, M.center_x + marker_pos_x, M.center_y, 0.5)
    end

    -- --- 2. INPUT HANDLING ---
    if IsMouseButtonDown(self.chop_button) then
        if not self.click_was_down then
            self.click_was_down = true
            
            if ui_active then
                self:AttemptChop()
            else
                local heldItemName = GetSelectedItemName()
                if current_herb_entity ~= nil and self:IsPlayerInRange() and heldItemName == "knife" then
                    self:AttemptChop() 
                end
            end
        end
    else
        self.click_was_down = false
    end
end

function M:IsPlayerInRange()
    if not player_entity then return false end
    local bx, by, bz = GetPosition(self.entity)
    local px, py, pz = GetPosition(player_entity)
    local dx = bx - px; local dy = by - py; local dz = bz - pz
    return math.sqrt(dx*dx + dy*dy + dz*dz) <= self.interaction_range
end

function M:StartChoppingSession(itemName)
    local prefabName = itemName .. "_Prefab"
    local bx, by, bz = GetPosition(self.entity)
    local newEntityID = SpawnPrefab(prefabName, bx, by + M.place_offset_y, bz + M.place_offset_z)

    if newEntityID ~= -1 and newEntityID ~= nil then
        current_herb_entity = newEntityID
        current_health = 5
        self:ClearUI() 
        SetMesh(current_herb_entity, self.mesh_raw)
        SetTexture(current_herb_entity, self.tex_raw)
        current_speed = M.marker_speed_base
    end
end

function M:AttemptChop()
    if not ui_active then
        self:StartMinigameUI()
        return
    end

    local diff = math.abs(marker_pos_x - zone_pos_x)
    local hit_threshold = M.zone_collision_width / 2

    if diff <= hit_threshold then
        self:ExecuteSuccessfulChop()
    else
        current_speed = current_speed - M.marker_speed_increase
        if current_speed < M.marker_speed_base then current_speed = M.marker_speed_base end
    end
end

function M:StartMinigameUI()
    ui_active = true
    marker_pos_x = 0 
    marker_dir = 1   
    
    if player_entity then
        local px, py, pz = GetPosition(player_entity)
        local cp, cy = GetCameraRot() 
        start_cam_pos = {x = px, y = py, z = pz}
        start_cam_rot = {pitch = cp, yaw = cy}

        local bx, by, bz = GetPosition(self.entity)
        target_cam_pos = {
            x = bx + self.FIXED_OFFSET_X,
            y = by + self.FIXED_OFFSET_Y,
            z = bz + self.FIXED_OFFSET_Z
        }
        target_cam_rot = { pitch = self.FIXED_TARGET_PITCH, yaw = self.FIXED_TARGET_YAW }

        if SetLinearVelocity then SetLinearVelocity(player_entity, 0, 0, 0) end
        SetCameraLock(true)
        SetUIFocused(true)
        is_camera_flying = true
        camera_flight_t = 0.0
    end
    
    current_speed = M.marker_speed_base
    zone_pos_x = math.random(-M.zone_safe_margin, M.zone_safe_margin)

    for _, prefabName in ipairs(M.ui_prefabs) do
        local z_layer = 0.1
        local spawn_x = M.center_x
        if prefabName == "UI_SkillBar_BG_Prefab" then z_layer = 0.0      
        elseif prefabName == "UI_SkillBar_Marker_Prefab" then z_layer = 0.5 
        elseif prefabName == "UI_SkillBar_Zone_Prefab" then spawn_x = M.center_x + zone_pos_x end
        
        local uiID = SpawnPrefab(prefabName, spawn_x, M.center_y, z_layer)
        if uiID ~= -1 then
            table.insert(spawned_ui_ids, uiID)
            if prefabName == "UI_SkillBar_Marker_Prefab" then active_marker_id = uiID end
            if prefabName == "UI_SkillBar_Zone_Prefab" then active_zone_id = uiID end
        end
    end
end

function M:ExecuteSuccessfulChop()
    current_health = current_health - 1
    current_speed = current_speed + M.marker_speed_increase
    
    PlayEntitySound(self.entity)
    -- ANIMATION IS TRIGGERED HERE
    if knife_entity then 
        PlayEntityAnimation(knife_entity, self.knife_animation_name) 
    end

    if current_health <= 0 then
        self:EndChoppingSession()
        return
    end

    if current_health == 3 then
        SetMesh(current_herb_entity, self.mesh_stage1)
        SetTexture(current_herb_entity, self.tex_stage1)
    elseif current_health == 1 then
        SetMesh(current_herb_entity, self.mesh_stage2)
        SetTexture(current_herb_entity, self.tex_stage2)
    end

    if active_zone_id then
        zone_pos_x = math.random(-M.zone_safe_margin, M.zone_safe_margin)
        SetPosition(active_zone_id, M.center_x + zone_pos_x, M.center_y, 0.1)
    end
end

function M:EndChoppingSession()
    if current_herb_entity then
        local bx, by, bz = GetPosition(self.entity)
        SpawnPrefab("chopped_herb_Prefab", bx, by + M.place_offset_y, bz + M.place_offset_z)
        DestroyEntity(current_herb_entity)
        current_herb_entity = nil
        self:ClearUI()
        SetColliderPromptText(self.entity, "Place ingredient")
    end
end

function M:ClearUI()
    if #spawned_ui_ids > 0 then
        for _, id in ipairs(spawned_ui_ids) do DestroyEntity(id) end
    end
    spawned_ui_ids = {} 
    ui_active = false
    active_marker_id = nil 
    active_zone_id = nil
    is_camera_flying = false
    
    if start_cam_pos and start_cam_rot and start_cam_pos.x ~= 0 then
        TeleportPlayer(start_cam_pos.x, start_cam_pos.y, start_cam_pos.z, 0, start_cam_rot.yaw, 0)
        SetCameraRot(start_cam_rot.pitch, start_cam_rot.yaw)
    end
    
    SetCameraLock(false) 
    SetUIFocused(false)
end

function ChoppingBoard_ResetStation()
    if ui_active then M:ClearUI() end
    if current_herb_entity and current_herb_entity ~= -1 then
        DestroyEntity(current_herb_entity)
        current_herb_entity = nil
    end
    if my_entity_id and SetColliderPromptText then
        SetColliderPromptText(my_entity_id, "Place ingredient")
    end
end

return M