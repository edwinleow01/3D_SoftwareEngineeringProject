local M = {}

-- =============================================================
-- [CONFIGURATION]
-- =============================================================
local CAULDRON_NAME = "Cauldron"
local name_stir = "CauldronStiring"       
local name_idle = "cauldronwithoutspoon" 

-- [CAMERA FLIGHT SETTINGS]
local FLIGHT_DURATION = 0.8
local FIXED_OFFSET_X = -0.5
local FIXED_OFFSET_Y = 1.5
local FIXED_OFFSET_Z = -3.0
local FIXED_TARGET_PITCH = 0.0
local FIXED_TARGET_YAW = 90.0

-- [RESULT SETTINGS]
local result_item_name = "strength_potion" 

-- [VISUAL ITEMS SETTINGS]
local orbit_radius_start = 0.5    
local orbit_speed_start = 1.5     
local orbit_radius_end = 0.0      
local orbit_speed_end = 15.0      

local bob_speed = 1.5       
local bob_height = 0.1     
local height_offset = 1.2   
local center_offset_x = -0.5 
local center_offset_z = -0.3

-- [PROGRESS BAR SETTINGS]
local prefab_frame = "ProgressBarFrame_Prefab"
local prefab_fill = "ProgressBarFill_Prefab"
local prefab_tick = "ProgressBarTick_Prefab"
local entity_ticks = {}
local active_tick_points = {}
local bar_length_max = 0.5
local bar_thickness = 0.2
local y_offset = 2.0
local x_offset_right = -0.5
local tune_x = 1.6
local tune_y = -0.04
local billboard_depth_adjust = 0.055

-- [STIRRING SETTINGS]
local stir_sensitivity = 0.005
local finish_delay = 1.0
local dir_threshold = 0.02
local anim_speed_abs = 20.0

-- [PER-RECIPE STIR PATTERNS]
-- dir = -1 means clockwise, dir = 1 means counter-clockwise.
-- The first segment must stay clockwise because the animation is authored that way.
local recipe_stir_patterns = {
    strength_potion = {
        { dir = -1, portion = 0.20 },
        { dir =  1, portion = 0.25 },
        { dir = -1, portion = 0.20 },
        { dir =  1, portion = 0.35 },
    },
    vitality_potion = {
        { dir = -1, portion = 0.15 },
        { dir =  1, portion = 0.20 },
        { dir = -1, portion = 0.25 },
        { dir =  1, portion = 0.15 },
        { dir = -1, portion = 0.25 },
    },
    warmth_potion = {
        { dir = -1, portion = 0.30 },
        { dir =  1, portion = 0.20 },
        { dir = -1, portion = 0.50 },
    },
    mending_potion = {
        { dir = -1, portion = 0.12 },
        { dir =  1, portion = 0.18 },
        { dir = -1, portion = 0.10 },
        { dir =  1, portion = 0.20 },
        { dir = -1, portion = 0.15 },
        { dir =  1, portion = 0.25 },
    },
    defense_potion = {
        { dir = -1, portion = 0.08 },
        { dir =  1, portion = 0.12 },
        { dir = -1, portion = 0.10 },
        { dir =  1, portion = 0.18 },
        { dir = -1, portion = 0.12 },
        { dir =  1, portion = 0.15 },
        { dir = -1, portion = 0.10 },
        { dir =  1, portion = 0.15 },
    },
}

-- [STIRRING ICON SETTINGS]
local stir_icon_name_ccw = "anti-rotatingIcon"
local stir_icon_name_cw  = "rotatingIcon"
local stir_icon_speed = 120.0           

-- =============================================================
-- [STATE VARIABLES]
-- =============================================================
local my_entity_id = nil
local is_active = false
local max_distance = 5.0
local player_id = nil
local accumulated_time = 0.0 
local current_orbit_angle = 0.0 

-- Camera Flight State
local is_camera_flying = false
local camera_flight_t = 0.0
local start_cam_pos = {x=0, y=0, z=0}
local start_cam_rot = {pitch=0, yaw=0}
local target_cam_pos = {x=0, y=0, z=0}
local target_cam_rot = {pitch=0, yaw=0}

-- Progress State
local current_finish_timer = 0.0
local is_finished = false
local progress = 0.0
local entity_frame = nil
local entity_fill = nil

-- Mouse Stirring State
local last_mouse_x = 0
local last_mouse_y = 0
local first_frame = true
local spoon_x = 0.0
local spoon_y = 100.0
local last_angle = 0.0
local last_required_dir = 0
local current_recipe_name = "strength_potion"
local active_recipe_pattern = nil

-- Ingredient State
local guid_stir = ""
local guid_idle = ""
local stored_ingredients = {} 
local visual_entities = {}    
local item_names = { [0] = "unchopped_herb", [1] = "chopped_herb", [2] = "pear" }

-- Player Interact Prompt
local last_prompt_state = ""

-- [AUDIO STATE]
local is_stirring_sound_active = false
local last_sound_trigger_time = 0.0

-- [UI LOG SETTINGS]
local prefab_log = "CauldronLog_Prefab"

-- [STIRRING ICON STATE]
local stir_icon_id_ccw = nil
local stir_icon_id_cw  = nil
local stir_icon_rotation = 0.0

-- =============================================================
-- [AUDIO HELPERS]
-- =============================================================
local function PlayStirSound()
    if (accumulated_time - last_sound_trigger_time) < 0.1 then return end

    local is_playing = false
    if Sound and Sound.IsPlaying then
        is_playing = Sound.IsPlaying("event:/Stirring")
    end

    if not is_playing then
        if Sound and Sound.Play then
            Sound.Play("event:/Stirring")
            last_sound_trigger_time = accumulated_time
        end
    end
end

local function StopStirSound()
    if Sound and Sound.IsPlaying and Sound.IsPlaying("event:/Stirring") then
        if Sound.Stop then
            Sound.Stop("event:/Stirring")
        end
    end
end

-- =============================================================
-- [HELPERS]
-- =============================================================
local function ResolveID(arg1, arg2)
    if type(arg1) == "table" and arg1.entity then 
        return arg1.entity 
    end
    if type(arg1) == "number" then return arg1 end
    if type(arg2) == "number" then return arg2 end
    return nil
end

local function GetMouseDelta()
    local mx, my = GetMousePosition() 
    if first_frame then
        last_mouse_x = mx
        last_mouse_y = my
        first_frame = false
        return 0, 0
    end
    local dx = mx - last_mouse_x
    local dy = my - last_mouse_y
    last_mouse_x = mx
    last_mouse_y = my
    return dx, dy
end

local function GetIngredientIDs()
    return stored_ingredients
end

local function SpawnLog(message)
    if SpawnPrefab and SetTextComponent then
        local logEntity = SpawnPrefab(prefab_log)
        if logEntity then
            SetTextComponent(logEntity, "text", message)
            print("[Cauldron] UI Log: " .. message)
        else
            print("[Cauldron] Failed to spawn log prefab!")
        end
    end
end

-- =============================================================
-- [STIRRING ICON HELPERS]
-- =============================================================
local function ShowStirIcon(required_dir)
    if required_dir == 1 then
        if stir_icon_id_ccw then
            SetIsUI(stir_icon_id_ccw, true)
        end
        if stir_icon_id_cw then
            SetIsUI(stir_icon_id_cw, false)
            SetRotation(stir_icon_id_cw, 0.0, 0.0, 0.0)
        end
    else
        if stir_icon_id_cw then
            SetIsUI(stir_icon_id_cw, true)
        end
        if stir_icon_id_ccw then
            SetIsUI(stir_icon_id_ccw, false)
            SetRotation(stir_icon_id_ccw, 0.0, 0.0, 0.0)
        end
    end
end

local function HideStirIcon()
    if stir_icon_id_cw then
        SetIsUI(stir_icon_id_cw, false)
        SetRotation(stir_icon_id_cw, 0.0, 0.0, 0.0)
    end
    if stir_icon_id_ccw then
        SetIsUI(stir_icon_id_ccw, false)
        SetRotation(stir_icon_id_ccw, 0.0, 0.0, 0.0)
    end
    stir_icon_rotation = 0.0
end

local function BuildNormalizedPattern(recipe_name)
    local source = recipe_stir_patterns[recipe_name] or recipe_stir_patterns["strength_potion"]
    local normalized = {}
    local total_portion = 0.0

    for i, segment in ipairs(source) do
        if i <= 8 then
            local dir = (segment.dir == 1) and 1 or -1
            local portion = tonumber(segment.portion) or 0.0
            if portion > 0.0 then
                if i == 1 then
                    dir = -1
                end
                total_portion = total_portion + portion
                table.insert(normalized, { dir = dir, portion = portion })
            end
        end
    end

    if #normalized == 0 then
        normalized = {
            { dir = -1, portion = 0.25 },
            { dir =  1, portion = 0.25 },
            { dir = -1, portion = 0.25 },
            { dir =  1, portion = 0.25 },
        }
        total_portion = 1.0
    end

    local cumulative = 0.0
    active_tick_points = {}

    for _, segment in ipairs(normalized) do
        segment.portion = segment.portion / total_portion
        cumulative = cumulative + segment.portion
        segment.progress_end = cumulative
    end

    for i = 1, (#normalized - 1) do
        table.insert(active_tick_points, normalized[i].progress_end)
    end

    return normalized
end

local function SetActiveRecipePattern(recipe_name)
    current_recipe_name = recipe_name or "strength_potion"
    active_recipe_pattern = BuildNormalizedPattern(current_recipe_name)
end

local function GetActiveRecipePattern()
    if not active_recipe_pattern then
        SetActiveRecipePattern(current_recipe_name)
    end
    return active_recipe_pattern
end

local function GetRequiredDir(p)
    local pattern = GetActiveRecipePattern()
    local clamped_progress = math.max(0.0, math.min(p, 0.9999))

    for _, segment in ipairs(pattern) do
        if clamped_progress < segment.progress_end then
            return segment.dir
        end
    end

    return pattern[#pattern].dir
end

local function UpdateTickVisibility()
    for i, tickEntity in ipairs(entity_ticks) do
        local boundary = active_tick_points[i] or 1.0
        local visible = progress < (boundary - 0.01)
        SetScale(
            tickEntity,
            visible and 0.01 or 0.0,
            visible and (bar_thickness * 0.9) or 0.0,
            visible and (bar_thickness * 0.9) or 0.0
        )
    end
end

-- =============================================================
-- 1. INIT
-- =============================================================
function M.on_init(arg1, arg2)
    local entity_id = ResolveID(arg1, arg2)
    if entity_id then
        my_entity_id = entity_id 
        guid_stir = GetAnimationGUID(name_stir)
        guid_idle = GetAnimationGUID(name_idle)
        SetAnimationAsset(my_entity_id, guid_idle)
        SetAnimationMode(my_entity_id, 0)
        last_angle = math.atan2(spoon_y, spoon_x)
    end
    player_id = FindEntityByName("Player")

    stir_icon_id_ccw = FindEntityByName(stir_icon_name_ccw)
    stir_icon_id_cw  = FindEntityByName(stir_icon_name_cw)

    SetActiveRecipePattern(current_recipe_name)
    HideStirIcon()

    if stir_icon_id_ccw then
        print("[Cauldron] CCW stirring icon found: " .. tostring(stir_icon_id_ccw))
    else
        print("[Cauldron] WARNING: Stirring icon entity '" .. stir_icon_name_ccw .. "' not found!")
    end

    if stir_icon_id_cw then
        print("[Cauldron] CW stirring icon found: " .. tostring(stir_icon_id_cw))
    else
        print("[Cauldron] WARNING: Stirring icon entity '" .. stir_icon_name_cw .. "' not found!")
    end

    if my_entity_id then
        SubscribeToEvent("ResetWorkstations", "Cauldron_ResetStation", my_entity_id)
    end
end

-- =============================================================
-- 2. INTERACT
-- =============================================================
function M.OnInteract(arg1, arg2)
    local id_to_use = my_entity_id or ResolveID(arg1, arg2) or FindEntityByName(CAULDRON_NAME)
    if id_to_use and not my_entity_id then my_entity_id = id_to_use end

    print("[MANUAL] OnInteract called for entity:", tostring(my_entity_id))

    local held_item_id = GetSelectedItem()

    -- [BRANCH A] ADDING INGREDIENT
    if held_item_id ~= -1 then
        table.insert(stored_ingredients, held_item_id)
        ConsumeSelectedItem()
        Sound.Play("event:/PlaceDownItem")
        local vis_id = -1
        if SpawnCauldronItem and id_to_use then
            local res = SpawnCauldronItem(id_to_use, held_item_id)
            if res then vis_id = res end
        end
        table.insert(visual_entities, vis_id)
        M.PrintContents()
        return 
    end

    -- [BRANCH B] TOGGLE STIRRING
    is_active = not is_active

    if is_active then
        local resultID = -1
        if CheckValidRecipe then
            resultID = CheckValidRecipe(stored_ingredients)
        else
            print("[Lua ERROR] CheckValidRecipe missing!")
        end

        if resultID ~= -1 then
            print("[Lua] Valid Recipe Found! Result ID: " .. resultID)
            
            if GetItemNameById then
                result_item_name = GetItemNameById(resultID)
            end

            SetActiveRecipePattern(result_item_name)

            -- Camera Flight Setup
            if player_id then
                local px, py, pz = GetPosition(player_id)
                local cp, cy = GetCameraRot() 
                start_cam_pos = {x = px, y = py, z = pz}
                start_cam_rot = {pitch = cp, yaw = cy}

                local mx, my, mz = GetPosition(id_to_use)
                target_cam_pos = {
                    x = mx + FIXED_OFFSET_X,
                    y = my + FIXED_OFFSET_Y,
                    z = mz + FIXED_OFFSET_Z
                }
                target_cam_rot = {
                    pitch = FIXED_TARGET_PITCH,
                    yaw   = FIXED_TARGET_YAW
                }

                if SetLinearVelocity then SetLinearVelocity(player_id, 0, 0, 0) end
                SetCameraLock(true)
                SetUIFocused(true)
                SetGravity(player_id, 0.0)
                is_camera_flying = true
                camera_flight_t = 0.0
            end

            -- Reset State
            progress = 0.0
            current_finish_timer = 0.0
            is_finished = false
            spoon_x = 0.0
            spoon_y = 100.0
            last_angle = math.atan2(spoon_y, spoon_x)
            
            if id_to_use then
                SetAnimationAsset(id_to_use, guid_stir)
                SetAnimationMode(id_to_use, 1) 
                M.SpawnBar(id_to_use)
            end

            ShowStirIcon(GetRequiredDir(0.0))
        else
            Sound.Play("event:/InvalidRecipe")
            print("[Lua] Invalid Recipe!")
            SpawnLog("Invalid Recipe!") 
            is_active = false 
        end
    else
        M.StopInteraction(id_to_use)
    end
end

function M.StopInteraction(entity_id)
    local id = entity_id or my_entity_id
    is_active = false
    is_finished = false
    is_camera_flying = false
    print("[Lua] Stirring Stopped")
    
    StopStirSound() 
    
    -- Return Player to Original Location
    if start_cam_pos and start_cam_rot then
        TeleportPlayer(start_cam_pos.x, start_cam_pos.y, start_cam_pos.z, 0, start_cam_rot.yaw, 0)
        SetCameraRot(start_cam_rot.pitch, start_cam_rot.yaw)
    end
    
    SetUIFocused(false)
    SetCameraLock(false)
    
    if id then
        SetAnimationAsset(id, guid_idle)
        SetAnimationMode(id, 0) 
    end
    if player_id then SetGravity(player_id, 1.0) end
    
    M.DestroyProgressBar()
    HideStirIcon()
end

-- =============================================================
-- 3. UPDATE LOOP
-- =============================================================
function M.on_update(arg1, arg2)
    local dt = 0.016
    if type(arg1) == "number" then dt = arg1 end
    if type(arg2) == "number" then dt = arg2 end

    accumulated_time = accumulated_time + dt
    if not debug_timer then debug_timer = 0.0 end
    debug_timer = debug_timer + dt

    if not my_entity_id then 
        my_entity_id = FindEntityByName(CAULDRON_NAME)
        if not my_entity_id then return end 
    end

    ---------------------------------------------------------
    -- 0. MANUAL CAMERA FLIGHT
    ---------------------------------------------------------
    if is_camera_flying then
        camera_flight_t = camera_flight_t + dt
        local t = math.min(camera_flight_t / FLIGHT_DURATION, 1.0)
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
        return -- Prevent stirring logic while camera is moving
    end

    ---------------------------------------------------------
    -- PHASE TRANSITION CHECK
    ---------------------------------------------------------
    if GetPhase then
        local current_phase = GetPhase()
        
        if current_phase ~= last_phase then
            if current_phase == 1 then 
                print("[Cauldron] Phase changed to Night. Resetting...")
                M.ClearCauldron()
                if is_active then
                    M.StopInteraction(my_entity_id)
                end
            end
            last_phase = current_phase
        end
    end

    ---------------------------------------------------------
    -- 1. PRE-CALCULATE STIRRING STATE
    ---------------------------------------------------------
    local is_stirring_correct_dir = false
    local px, py, pz, cx, cy, cz

    if is_active and player_id then
        px, py, pz = GetPosition(player_id)
        cx, cy, cz = GetPosition(my_entity_id)
        
        if px and cx then
            local dist = math.sqrt((px - cx)^2 + (py - cy)^2 + (pz - cz)^2)
            if dist > max_distance then
                M.StopInteraction(my_entity_id)
            elseif not is_finished then
                -- Mouse Stirring Math
                local dx, dy = GetMouseDelta()
                spoon_x = spoon_x + dx
                spoon_y = spoon_y + dy

                local len = math.sqrt(spoon_x*spoon_x + spoon_y*spoon_y)
                if len > 0.1 then
                    spoon_x = (spoon_x / len) * 100.0
                    spoon_y = (spoon_y / len) * 100.0
                end

                local current_angle = math.atan2(spoon_y, spoon_x)
                local angle_delta = current_angle - last_angle
                local PI = 3.14159265359

                if angle_delta > PI then angle_delta = angle_delta - (2 * PI) end
                if angle_delta < -PI then angle_delta = angle_delta + (2 * PI) end
                last_angle = current_angle

                local required_dir = GetRequiredDir(progress)
                ShowStirIcon(required_dir)

                if required_dir ~= last_required_dir then
                    last_required_dir = required_dir
                    SetAnimationSettings(my_entity_id, anim_speed_abs * required_dir, true, true)
                end

                if required_dir == 1 then
                    is_stirring_correct_dir = (angle_delta < -dir_threshold)
                else
                    is_stirring_correct_dir = (angle_delta > dir_threshold)
                end

                if is_stirring_correct_dir then
                    progress = progress + (math.abs(angle_delta) * stir_sensitivity)
                    SetAnimationSettings(my_entity_id, anim_speed_abs * required_dir, true, true)
                    PlayStirSound()
                else
                    SetAnimationSettings(my_entity_id, 0.0, true, true)
                    StopStirSound()
                end

                if progress >= 1.0 then
                    progress = 1.0
                    is_finished = true
                    current_finish_timer = finish_delay
                end
            end
        end
    end

    ---------------------------------------------------------
    -- 2. STIRRING ICON ROTATION
    ---------------------------------------------------------
    if is_active then
        local required_dir = GetRequiredDir(progress)

        stir_icon_rotation = stir_icon_rotation + (stir_icon_speed * dt * required_dir)

        if stir_icon_rotation >= 360.0 then
            stir_icon_rotation = stir_icon_rotation - 360.0
        elseif stir_icon_rotation <= -360.0 then
            stir_icon_rotation = stir_icon_rotation + 360.0
        end

        if required_dir == -1 and stir_icon_id_cw then
            SetRotation(stir_icon_id_cw, 0.0, 0.0, stir_icon_rotation)
        elseif required_dir == 1 and stir_icon_id_ccw then
            SetRotation(stir_icon_id_ccw, 0.0, 0.0, stir_icon_rotation)
        end
    end

    ---------------------------------------------------------
    -- 3. DYNAMIC TEXT & ORBIT SPEED
    ---------------------------------------------------------
    if SetInteractionText then
        local held_item = GetSelectedItem() 
        local desired_text = is_active and "Stirring..." or (held_item ~= -1 and "Add Ingredient" or "Stir Cauldron")
        if desired_text ~= last_prompt_state then
            SetInteractionText(my_entity_id, desired_text)
            last_prompt_state = desired_text
        end
    end

    -- Update Orbit
    if is_active and is_stirring_correct_dir and not is_finished then
        local dynamic_speed = orbit_speed_start + (orbit_speed_end - orbit_speed_start) * progress
        local required_dir = GetRequiredDir(progress)
        current_orbit_angle = current_orbit_angle + (dt * dynamic_speed * required_dir)
    elseif not is_active then
        current_orbit_angle = current_orbit_angle + (dt * orbit_speed_start)
    end

    ---------------------------------------------------------
    -- 4. ITEM ANIMATION & ROTATION
    ---------------------------------------------------------
    local total_count = #visual_entities

    for i = total_count, 1, -1 do
        local vis_id = visual_entities[i]
        local should_remove = false

        if vis_id > 0 and IsItemZipping and IsItemZipping(vis_id) then
            should_remove = true
            print("[Cauldron] Releasing " .. vis_id .. " to Player (Zip Started)")
        end

        if not should_remove and IsEntityValid and not IsEntityValid(vis_id) then
            should_remove = true
            print("[Cauldron] Cleaning up invalid entity: " .. vis_id)
        end

        if should_remove then
            table.remove(visual_entities, i)
            table.remove(stored_ingredients, i)
            if #visual_entities == 0 and SetInteractionText then
                SetInteractionText(my_entity_id, "Stir Cauldron")
                last_prompt_state = "Stir Cauldron"
            end
        else
            local current_count = #visual_entities
            local angle_step = (2 * math.pi) / current_count
            local item_angle = current_orbit_angle + ((i - 1) * angle_step)
            
            local dynamic_radius = is_active and (orbit_radius_start + (orbit_radius_end - orbit_radius_start) * progress) or orbit_radius_start
            
            local offset_x = math.cos(item_angle) * dynamic_radius
            local offset_z = math.sin(item_angle) * dynamic_radius
            
            local vertical_bob = math.sin((accumulated_time * bob_speed) + i) * (is_active and 0.0 or bob_height)
            
            local raw_cx, raw_cy, raw_cz = GetPosition(my_entity_id)
            if raw_cx then 
                SetPosition(vis_id, 
                    raw_cx + center_offset_x + offset_x, 
                    raw_cy + height_offset + vertical_bob, 
                    raw_cz + center_offset_z + offset_z
                )

                if is_active then
                    if is_stirring_correct_dir and not is_finished then
                        SetRotation(vis_id, 0.0, accumulated_time * 250.0, 0.0)
                    else
                        SetRotation(vis_id, 0.0, GetRotation(vis_id), 0.0) 
                    end
                else
                    SetRotation(vis_id, 0.0, accumulated_time * 50.0, 0.0)
                end
            end
        end
    end

    ---------------------------------------------------------
    -- 5. UI & FINISHING
    ---------------------------------------------------------
    if is_active and entity_fill and entity_frame and px then
        M.UpdateBillboard(px, pz, cx, cz, cy)

        if is_finished then
            current_finish_timer = current_finish_timer - dt
            SetAnimationSettings(my_entity_id, 0.0, true, true) 
            if current_finish_timer <= 0 then M.OnPotionComplete() end
        else
            SetScale(entity_fill, progress * bar_length_max, bar_thickness * 0.9, bar_thickness * 0.9)
        end
    end

    if is_active then
        UpdateTickVisibility()
    end

end

-- =============================================================
-- [HELPER FUNCTIONS]
-- =============================================================

function M.SpawnBar(cauldronID)
    if SpawnPrefab then
        entity_frame = SpawnPrefab(prefab_frame, cauldronID)
        if entity_frame then
            SetScale(entity_frame, bar_length_max, bar_thickness, bar_thickness * 5)
        end

        entity_fill = SpawnPrefab(prefab_fill, cauldronID)
        if entity_fill then
            SetScale(entity_fill, 0.0, bar_thickness, bar_thickness)
        end

        entity_ticks = {}
        for _, _ in ipairs(active_tick_points) do
            local tick = SpawnPrefab(prefab_tick, cauldronID)
            if tick then
                SetScale(tick, 0.01, bar_thickness * 0.9, bar_thickness * 0.9)
                table.insert(entity_ticks, tick)
            end
        end
    else
        print("[Lua ERROR] SpawnPrefab function missing!")
    end
end

function M.DestroyProgressBar()
    if entity_frame then 
        DestroyEntity(entity_frame) 
        entity_frame = nil
    end
    if entity_fill then 
        DestroyEntity(entity_fill) 
        entity_fill = nil
    end

    for i, tick in ipairs(entity_ticks) do
        if tick then
            DestroyEntity(tick)
        end
        entity_ticks[i] = nil
    end

    entity_ticks = {}
end

function M.UpdateBillboard(px, pz, cx, cz, cy)

    local dx = px - cx
    local dz = pz - cz
    local angle_rad = math.atan2(dx, dz)
    local angle_deg = math.deg(angle_rad)

    local frame_pos_x = cx + x_offset_right
    local frame_pos_y = cy + y_offset
    local frame_pos_z = cz

    local right_angle = angle_rad + (math.pi / 2.0)
    local side_dist = (bar_length_max * tune_x)

    local depth_offset_x = billboard_depth_adjust * math.sin(angle_rad)
    local depth_offset_z = billboard_depth_adjust * math.cos(angle_rad)

    if entity_frame then
        SetPosition(entity_frame, frame_pos_x, frame_pos_y, frame_pos_z)
        SetRotation(entity_frame, 0.0, angle_deg + 180.0, 0.0)
    end

    if entity_fill then
        local side_offset_x = side_dist * math.sin(right_angle)
        local side_offset_z = side_dist * math.cos(right_angle)

        local fill_x = frame_pos_x + side_offset_x + depth_offset_x
        local fill_y = frame_pos_y + tune_y
        local fill_z = frame_pos_z + side_offset_z + depth_offset_z

        SetRotation(entity_fill, 0.0, angle_deg + 180.0, 0.0)
        SetPosition(entity_fill, fill_x, fill_y, fill_z)
    end

    local function SetTickPosition(tickEntity, t)
        if not tickEntity then return end

        local tick_dist = side_dist * (1.0 - 2.0 * t)
        local tick_offset_x = tick_dist * math.sin(right_angle)
        local tick_offset_z = tick_dist * math.cos(right_angle)

        local tick_x = frame_pos_x + tick_offset_x + depth_offset_x
        local tick_y = frame_pos_y + tune_y
        local tick_z = frame_pos_z + tick_offset_z + depth_offset_z

        SetPosition(tickEntity, tick_x, tick_y, tick_z)
        SetRotation(tickEntity, 0.0, angle_deg + 180.0, 0.0)
    end

    for i, tickEntity in ipairs(entity_ticks) do
        SetTickPosition(tickEntity, active_tick_points[i] or 0.5)
    end
end

-- =============================================================
-- UTILITY
-- =============================================================
function M.OnPotionComplete()
    M.StopInteraction(my_entity_id)  
    M.ClearCauldron()
    print("[Lua] Potion Complete! Ingredients Consumed.")

    if Sound and Sound.Play then
        Sound.Play("event:/RecipeComplete")
    end

    local cx, cy, cz = GetPosition(my_entity_id)
    if cx then
        local spawn_x = cx + center_offset_x
        local spawn_y = cy + height_offset 
        local spawn_z = cz + center_offset_z

        local prefabName = result_item_name .. "_Prefab"
        
        if SpawnPrefab then
            local potionID = SpawnPrefab(prefabName, spawn_x, spawn_y, spawn_z)
            print("[Lua] Spawned Potion Prefab: " .. prefabName .. " (ID: " .. tostring(potionID) .. ")")
        else
            print("[Lua ERROR] SpawnPrefab missing!")
        end
    end
end

function M.PrintContents()
    local content_string = "Cauldron Contents: "
    if #stored_ingredients == 0 then print(content_string .. "Empty") return end
    for i, id in ipairs(stored_ingredients) do
        local name = item_names[id] or tostring(id)
        content_string = content_string .. "[" .. name .. "] "
    end
    print(content_string)
end

function M.ClearCauldron()
    stored_ingredients = {}
    for i, vis_id in ipairs(visual_entities) do
        if DestroyEntity then DestroyEntity(vis_id) end
    end
    visual_entities = {}
    print("[Lua] Cauldron Cleared")
end

-- The Event Callback
function Cauldron_ResetStation()
    if is_active then
        M.StopInteraction(my_entity_id)
    end
    M.ClearCauldron()
    print("[Cauldron] Station Reset for new day.")
end

return M