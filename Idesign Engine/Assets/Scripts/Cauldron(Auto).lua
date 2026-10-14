local M = {}

-- =============================================================
-- [CONFIGURATION]
-- =============================================================
local CAULDRON_NAME = "Cauldron(Auto)"
local name_stir = "CauldronStiring"       
local name_idle = "cauldronwithoutspoon" 

-- [RESULT SETTINGS]
-- The name of the item to spawn when done.
-- The script will look for "strength_potion_Prefab" (automatically adds "_Prefab")
local result_item_name = "strength_potion" 

-- [VISUAL ITEMS SETTINGS]
local orbit_radius_start = 0.5    
local orbit_speed_start = 1.5     
local orbit_radius_end = 0.0      -- Merge to center
local orbit_speed_end = 15.0      

local bob_speed = 1.5       
local bob_height = 0.1     
local height_offset = 1.2   
local center_offset_x = -0.5 
local center_offset_z = -0.3

-- [PROGRESS BAR SETTINGS]
local prefab_frame = "ProgressBarFrame_Prefab"
local prefab_fill = "ProgressBarFill_Prefab"
local bar_length_max = 0.5
local bar_thickness = 0.2
local y_offset = 1.5
local x_offset_right = -0.5
local tune_x = 1.6
local tune_y = -0.04
local billboard_depth_adjust = 0.055

-- [STIRRING SETTINGS]
local stir_sensitivity = 0.005
local finish_delay = 1.0
local dir_threshold = 0.02
local dir_segments = 4
local anim_speed_abs = 20.0
local is_auto_cauldron = true
local auto_stir_duration = 10.0   -- fallback default

-- [POTION BREW DURATIONS] (seconds) - tune these to your liking
local potion_brew_durations = {
    ["strength_potion"] = 10.0,
    ["mending_potion"]  = 10.0,
    ["defense_potion"]  = 10.0,
    ["vitality_potion"] = 10.0,
    ["warmth_potion"]   = 10.0,
}

-- [STIRRING ICON SETTINGS]
local stir_icon_name_ccw = ""
local stir_icon_name_cw  = ""
local stir_icon_speed = 120.0           -- Degrees per second (tune this)

-- =============================================================
-- [STATE VARIABLES]
-- =============================================================
local my_entity_id = nil
local is_active = false
local max_distance = 5.0
local player_id = nil
local accumulated_time = 0.0 
local current_orbit_angle = 0.0 

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

-- Ingredient State
local guid_stir = ""
local guid_idle = ""
local stored_ingredients = {} 
local visual_entities = {}    
local item_names = { [0] = "unchopped_herb", [1] = "chopped_herb", [2] = "pear" }
local auto_min_ingredients = 2
local brew_result_name = ""

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

-- [TIMER TEXT STATE]
local timer_text_entity = nil
local brew_time_left    = 0.0

-- [LOCK/VISIBILITY STATE]
local is_unlocked = false
local origin_x = nil
local origin_y = nil
local origin_z = nil

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
    if required_dir == -1 then
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
    if stir_icon_id_ccw then
        SetIsUI(stir_icon_id_ccw, false)
        SetRotation(stir_icon_id_ccw, 0.0, 0.0, 0.0)
    end
    if stir_icon_id_cw then
        SetIsUI(stir_icon_id_cw, false)
        SetRotation(stir_icon_id_cw, 0.0, 0.0, 0.0)
    end
    stir_icon_rotation = 0.0
end

-- =============================================================
-- [LOCK VISIBILITY HELPER]
-- =============================================================
local function SetCauldronVisible(visible)
    if not my_entity_id then return end
    if visible then
        if origin_x then
            SetPosition(my_entity_id, origin_x, origin_y, origin_z)
        end
    else
        local cx, cy, cz = GetPosition(my_entity_id)
        if cx then
            SetPosition(my_entity_id, cx, -9999.0, cz)
        end
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

        -- Capture origin ONCE before any hide ops
        origin_x, origin_y, origin_z = GetPosition(my_entity_id)

        -- Apply initial lock state
        is_unlocked = IsCauldronUnlocked()
        SetCauldronVisible(is_unlocked)
    end
    player_id = FindEntityByName("Player")

    -- Find the stirring icon entity and hide it at start
    stir_icon_id_ccw = FindEntityByName(stir_icon_name_ccw)
    stir_icon_id_cw  = FindEntityByName(stir_icon_name_cw)

    HideStirIcon()

    -- Find the brew timer text entity and hide it at start
    timer_text_entity = FindEntityByName("AutoCauldronText")
    if timer_text_entity then
        SetTextComponent(timer_text_entity, "text", "")
        SetTextComponent(timer_text_entity, "opacity", 0.0)
        print("[Cauldron] Timer text entity found: " .. tostring(timer_text_entity))
    else
        print("[Cauldron] WARNING: AutoCauldronText entity not found!")
    end

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
end

-- =============================================================
-- 2. INTERACT
-- =============================================================
local function StartAutoBrew(id_to_use, resultID)
    is_active = true
    progress = 0.0
    current_finish_timer = 0.0
    is_finished = false
    spoon_x = 0.0
    spoon_y = 100.0
    last_angle = math.atan2(spoon_y, spoon_x)
    last_required_dir = 0

    if GetItemNameById then
        result_item_name = GetItemNameById(resultID)
    end

    -- Look up per-potion duration; fall back to default if not found
    local looked_up = potion_brew_durations[result_item_name]
    auto_stir_duration = looked_up or 10.0
    print("[Cauldron] Brew duration for '" .. tostring(result_item_name) .. "': " .. auto_stir_duration .. "s")

    if id_to_use then
        SetAnimationAsset(id_to_use, guid_stir)
        SetAnimationMode(id_to_use, 1)
        M.SpawnBar(id_to_use)
    end

    ShowStirIcon(GetRequiredDir(0.0))
    PlayStirSound()

    -- Start timer text countdown
    brew_time_left = auto_stir_duration
    if timer_text_entity then
        SetTextComponent(timer_text_entity, "opacity", 1.0)
    end
end


function M.OnInteract(arg1, arg2)

    -- Check if auto cauldron is unlocked
    if not IsCauldronUnlocked() then
        SpawnLog("Auto Cauldron Locked!")
        Sound.Play("event:/InvalidRecipe") 
        print("[AUTO] Cauldron not unlocked yet.")
        return
    end

    local id_to_use = ResolveID(arg1, arg2) or my_entity_id or FindEntityByName(CAULDRON_NAME)
    if id_to_use then my_entity_id = id_to_use end

    local held_item_id = GetSelectedItem()

    -------------------------------------------------
    -- 1. ADD INGREDIENT
    -------------------------------------------------
    if held_item_id ~= -1 then

        if is_active then
            print("[AUTO] Cannot add ingredients while brewing.")
            return
        end

        table.insert(stored_ingredients, held_item_id)
        ConsumeSelectedItem()
        Sound.Play("event:/PlaceDownItem")

        local vis_id = -1
        if SpawnCauldronItem then
            vis_id = SpawnCauldronItem(my_entity_id, held_item_id)
        end

        table.insert(visual_entities, vis_id)

        M.PrintContents()

        -- IMPORTANT: DO NOT START BREW HERE
        return
    end

    -------------------------------------------------
    -- 2. START BREW (EMPTY HAND INTERACT)
    -------------------------------------------------
    if is_active then
        print("[AUTO] Already brewing.")
        return
    end

    if #stored_ingredients == 0 then
        print("[AUTO] No ingredients.")
        return
    end

    local resultID = -1
    if CheckValidRecipe then
        resultID = CheckValidRecipe(stored_ingredients)
    end

    if resultID ~= -1 then
        print("[AUTO] Player started auto brew.")
        StartAutoBrew(my_entity_id, resultID)
    else
        Sound.Play("event:/InvalidRecipe")
        SpawnLog("Invalid Recipe!")
    end

end

function M.StopInteraction(entity_id)
    local id = entity_id or my_entity_id
    is_active = false
    is_finished = false
    print("[Lua] Stirring Stopped")
    
    StopStirSound() 
    
    SetCameraLock(false)
    if id then
        SetAnimationAsset(id, guid_idle)
        SetAnimationMode(id, 0) 
    end
    if player_id then SetGravity(player_id, 1.0) end
    
    M.DestroyProgressBar()

    -- Hide the stirring icon
    HideStirIcon()

    -- Hide timer text
    brew_time_left = 0.0
    if timer_text_entity then
        SetTextComponent(timer_text_entity, "text", "")
        SetTextComponent(timer_text_entity, "opacity", 0.0)
    end
end

function GetRequiredDir(p)
    local seg = math.floor(math.min(p, 0.9999) * dir_segments)
    return (seg % 2 == 0) and -1 or 1
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
    -- UNLOCK VISIBILITY POLL
    ---------------------------------------------------------
    local unlocked_now = IsCauldronUnlocked()
    if unlocked_now ~= is_unlocked then
        is_unlocked = unlocked_now
        SetCauldronVisible(is_unlocked)
        last_prompt_state = ""  -- force prompt refresh on unlock
    end

    -- Skip all further updates if locked
    if not is_unlocked then return end

    ---------------------------------------------------------
    -- 1. PRE-CALCULATE STIRRING STATE
    ---------------------------------------------------------
    local is_stirring_correct_dir = false
    local px, py, pz, cx, cy, cz

    if is_active then
        if not is_finished then
            local required_dir = GetRequiredDir(progress)
            ShowStirIcon(required_dir)

            if required_dir ~= last_required_dir then
                last_required_dir = required_dir
                SetAnimationSettings(my_entity_id, anim_speed_abs * required_dir, true, true)
            end

            is_stirring_correct_dir = true
            progress = progress + (dt / auto_stir_duration)

            -- Tick and display countdown
            brew_time_left = brew_time_left - dt
            if brew_time_left < 0.0 then brew_time_left = 0.0 end
            if timer_text_entity then
                local secs = math.ceil(brew_time_left)
                local mins = math.floor(secs / 60)
                local rem  = secs % 60
                SetTextComponent(timer_text_entity, "text", string.format("%d:%02d", mins, rem))
            end

            SetAnimationSettings(my_entity_id, anim_speed_abs * required_dir, true, true)
            PlayStirSound()

            if progress >= 1.0 then
                progress = 1.0
                is_finished = true
                current_finish_timer = finish_delay
            end
        end
    end

    ---------------------------------------------------------
    -- 2. STIRRING ICON ROTATION
    ---------------------------------------------------------
    if is_active then
        local required_dir = GetRequiredDir(progress)

        stir_icon_rotation = stir_icon_rotation - (stir_icon_speed * dt * required_dir)

        if stir_icon_rotation >= 360.0 then
            stir_icon_rotation = stir_icon_rotation - 360.0
        elseif stir_icon_rotation <= -360.0 then
            stir_icon_rotation = stir_icon_rotation + 360.0
        end

        if required_dir == -1 and stir_icon_id_ccw then
            SetRotation(stir_icon_id_ccw, 0.0, 0.0, stir_icon_rotation)
        elseif required_dir == 1 and stir_icon_id_cw then
            SetRotation(stir_icon_id_cw, 0.0, 0.0, stir_icon_rotation)
        end
    end

    ---------------------------------------------------------
    -- 3. DYNAMIC TEXT & ORBIT SPEED
    ---------------------------------------------------------
    if SetInteractionText then
        local held_item = GetSelectedItem() 
        local desired_text = is_active and "Brewing..." or (held_item ~= -1 and "Add Ingredient" or "Auto Cauldron")
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
                SetInteractionText(my_entity_id, "Auto Cauldron")
                last_prompt_state = "Auto Cauldron"
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
    if is_active and entity_fill and entity_frame then
        local px, py, pz = GetPosition(player_id)
        local cx, cy, cz = GetPosition(my_entity_id)

        if px and cx then
            M.UpdateBillboard(px, pz, cx, cz, cy)
        end

        if is_finished then
            current_finish_timer = current_finish_timer - dt
            SetAnimationSettings(my_entity_id, 0.0, true, true) 
            if current_finish_timer <= 0 then M.OnPotionComplete() end
        else
            SetScale(entity_fill, progress * bar_length_max, bar_thickness * 0.9, bar_thickness * 0.9)
        end
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
end

function M.UpdateBillboard(px, pz, cx, cz, cy)
    local dx = px - cx
    local dz = pz - cz
    local angle_rad = math.atan2(dx, dz)
    local angle_deg = math.deg(angle_rad)

    local frame_pos_x = cx + x_offset_right
    local frame_pos_y = cy + y_offset
    local frame_pos_z = cz

    if entity_frame then
        SetPosition(entity_frame, frame_pos_x, frame_pos_y, frame_pos_z)
        SetRotation(entity_frame, 0.0, angle_deg + 180.0, 0.0)
    end

    if entity_fill then
        SetRotation(entity_fill, 0.0, angle_deg + 180.0, 0.0)

        local right_angle = angle_rad + (math.pi / 2.0)
        local side_dist = (bar_length_max * tune_x)
        local side_offset_x = side_dist * math.sin(right_angle)
        local side_offset_z = side_dist * math.cos(right_angle)

        local depth_offset_x = billboard_depth_adjust * math.sin(angle_rad)
        local depth_offset_z = billboard_depth_adjust * math.cos(angle_rad)

        local fill_x = frame_pos_x + side_offset_x + depth_offset_x
        local fill_y = frame_pos_y + tune_y
        local fill_z = frame_pos_z + side_offset_z + depth_offset_z

        SetPosition(entity_fill, fill_x, fill_y, fill_z)
    end
end

-- =============================================================
-- UTILITY
-- =============================================================
function M.OnPotionComplete()
    M.StopInteraction(my_entity_id)  -- This also calls HideStirIcon via StopInteraction
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

return M