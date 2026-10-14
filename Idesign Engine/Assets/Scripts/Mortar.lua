SubscribeToEvent("ResetWorkstations2", "MortarReset", FindEntityByName("Mortar"))

local Mortar = {}

-- --- PREFABS ---
local PREFAB_CIRCLE = "TapCircle_Prefab"
local PREFAB_GOOD   = "GoodPound_Prefab"
local PREFAB_SPINNER = "Spinner_Prefab"
local PREFAB_MEDIUM = "MediumPound_Prefab"
local PREFAB_POOR   = "PoorPound_Prefab"
local PREFAB_FRAME  = "ProgressBarFrame_Prefab"
local PREFAB_FILL   = "ProgressBarFill_Prefab"

-- --- CONFIGURATION ---
local DECAY_RATE = 5.0 
local MORTAR_CAM_HEIGHT = 2.5
local TARGET_CAM_PITCH = -70.0
local FLIGHT_DURATION = 0.8
local PLAYER_Y_OFFSET = 0.0

-- How far away from the mortar the camera should sit
local FIXED_OFFSET_X = 0.0   -- 0 means centered on X
local FIXED_OFFSET_Z = 1.0   -- 2.5 units "back" on Z
local FIXED_OFFSET_Y = 3.0   -- 4.0 units "up" on Y

-- The fixed angles the camera should have once it arrives
local FIXED_TARGET_PITCH = -70.0 -- Looking down at an angle
local FIXED_TARGET_YAW   = 270.0 -- Facing the workstation

-- --- INTERNAL STATE ---
local is_playing = false
local is_camera_flying = false
local camera_flight_t = 0.0
local start_cam_pos = {x=0, y=0, z=0}
local start_cam_rot = {pitch=0, yaw=0}
local target_cam_pos = {x=0, y=0, z=0}

local spawn_timer = 0.0
local active_circles = {} 
local feedback_texts = {} 
local click_was_down = false 
local my_entity_id = nil

local last_phase = -1 -- Initialize to -1 so it triggers on first check

local FINISH_DELAY = 1.0  -- Seconds to wait after finishing before closing the UI

-- [INGREDIENT STATE]
local stored_ingredients = {}
local visual_entities = {}
local MAX_INGREDIENTS = 3
local OFFSET_X_START, SPACING_X = -1.5, -1 
local OFFSET_Y, OFFSET_Z = 0.1, 0.0
local ROT_X, ROT_Y, ROT_Z = 180.0, 0.0, 0.0

-- [POUNDING/SPINNING STATE]
local current_progress, MAX_PROGRESS = 0, 100
local POINTS_GOOD, POINTS_MEDIUM, POINTS_POOR = 20, 10, 5
local CLICK_RADIUS = 75 
local is_spinning, spin_progress = false, 0.0
local SPIN_REQUIREMENT = math.pi * 10 
local spinner_visual_id, last_angle = nil, 0.0
local last_mx, last_my = nil, nil
local spoon_x, spoon_y = 0.0, 100.0

-- [2D UI]
local e_progress_frame, e_progress_fill = nil, nil
local UI_WIDTH, UI_HEIGHT = 1920.0, 1080.0
local BAR_WIDTH_PX, BAR_HEIGHT_PX, BAR_Y_POS = 600, 40, 400


-- =============================================================
-- [AUDIO HELPERS]
-- =============================================================
local function PlayGrinderLoop()
    local is_looping = false
    if Sound and Sound.IsPlaying then
        is_looping = Sound.IsPlaying("event:/GrinderLoop")
    end

    if not is_looping then
        if Sound and Sound.Play then
            Sound.Play("event:/GrinderLoop")
        end
    end
end

local function StopGrinderLoop()
    if Sound and Sound.IsPlaying and Sound.IsPlaying("event:/GrinderLoop") then
        if Sound.Stop then
            Sound.Stop("event:/GrinderLoop")
        end
    end
end


function Mortar:on_init(arg1, arg2)
    -- BULLETPROOF ID GRABBER
    if type(arg1) == "table" and arg1.entity then my_entity_id = arg1.entity
    elseif type(arg1) == "number" then my_entity_id = arg1
    elseif type(arg2) == "number" then my_entity_id = arg2
    else my_entity_id = FindEntityByName("Mortar") end

    math.randomseed(os.time()) 
end
-- --- UPDATE ---
function Mortar.on_update(self, dt)
    -- 1. MANUAL CAMERA FLIGHT
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
        return 
    end

    if GetPhase then
        local current_phase = GetPhase()
        if current_phase ~= last_phase then
            -- If current_phase is 1 (Night/Reset Phase), trigger the reset
            if current_phase == 1 then 
                print("[Mortar] Phase changed. Resetting station...")
                -- Call your existing reset logic
                MortarReset() 
            end
            last_phase = current_phase
        end
    end

    -- 2. INGREDIENT VISUALS & DYNAMIC REMOVAL
    if my_entity_id then
        local cx, cy, cz = GetPosition(my_entity_id)
        
        for i = #visual_entities, 1, -1 do
            local vis_id = visual_entities[i]

            -- Check if the item is being picked up by the player
            if vis_id and vis_id > 0 and IsItemZipping and IsItemZipping(vis_id) then
                table.remove(visual_entities, i)
                table.remove(stored_ingredients, i)
                print("[Mortar] Ingredient removed. Bowl count: " .. #stored_ingredients)
                
                -- If we were mid-game, check if the recipe is still valid
                if is_playing then
                    local resID = CheckValidRecipe(stored_ingredients)
                    if resID == -1 then
                        print("[Mortar] Recipe broken! Stopping minigame.")
                        Mortar.StopMinigame(my_entity_id, false)
                    end
                end
            elseif vis_id > 0 and cx then
                -- Standard Position Update
                local current_x_offset = OFFSET_X_START + ((i - 1) * SPACING_X)
                SetPosition(vis_id, cx + current_x_offset, cy + OFFSET_Y, cz + OFFSET_Z)
                
                local item_id = stored_ingredients[i]
                local itemName = GetItemNameById(item_id)
                
                if string.find(itemName, "Herb") and not string.find(itemName, "Chopped") then
                    SetRotation(vis_id, 180.0, 0.0, 0.0) 
                else
                    SetRotation(vis_id, 0.0, 0.0, 0.0)
                end
            end
        end
    end

    if not is_playing then return end

    -- 3. PHASE 1: POUNDING
    if not is_spinning then
        current_progress = math.max(0, current_progress - (DECAY_RATE * dt))
        
        if e_progress_fill then
            local ratio = math.min(current_progress / MAX_PROGRESS, 1.0)
            SetScale(e_progress_fill, ratio * BAR_WIDTH_PX, BAR_HEIGHT_PX * 0.8, 1.0)
            SetPosition(e_progress_fill, (UI_WIDTH / 2) - (BAR_WIDTH_PX / 2) + (ratio * BAR_WIDTH_PX / 2), BAR_Y_POS, 0.9)
        end

        spawn_timer = spawn_timer + dt
        if spawn_timer >= 1.0 then
            spawn_timer = 0
            local rx, ry = math.random(200, 1720), math.random(200, 880)
            local cid = SpawnPrefab(PREFAB_CIRCLE, rx, ry, 0.5)
            if cid and cid ~= -1 then
                local sx, sy, sz = GetScale(cid)
                table.insert(active_circles, { id = cid, life = 0.0, x = rx, y = ry, base_sx = sx, base_sy = sy, base_sz = sz })
            end
        end

        for i = #active_circles, 1, -1 do
            local c = active_circles[i]
            c.life = c.life + dt
            if c.life >= 3.0 then 
                DestroyEntity(c.id); table.remove(active_circles, i)
            else
                local s = math.max(1.0 - (c.life / 3.0), 0.05)
                SetScale(c.id, c.base_sx * s, c.base_sy * s, c.base_sz * s)
            end
        end

        if IsMouseButtonDown(0) and not click_was_down then
            click_was_down = true
            local uvx, uvy = GetMouseUV()
            local mx, my = uvx * UI_WIDTH, UI_HEIGHT - (uvy * UI_HEIGHT)
            for i = #active_circles, 1, -1 do
                local circ = active_circles[i]
                local dyn_rad = CLICK_RADIUS * math.max(1.0 - (circ.life / 3.0), 0.05)
                if math.sqrt((mx - circ.x)^2 + (my - circ.y)^2) <= dyn_rad then
                    current_progress = math.min(current_progress + ((circ.life <= 1.0) and POINTS_GOOD or (circ.life <= 2.0 and POINTS_MEDIUM or POINTS_POOR)), MAX_PROGRESS)
                    Sound.Play("event:/GrinderHit")

                    local prefab = (circ.life <= 1.0 and PREFAB_GOOD or (circ.life <= 2.0 and PREFAB_MEDIUM or PREFAB_POOR))
                    local fb_id = SpawnPrefab(prefab, mx, my + 20, 0.6)
                    if fb_id and fb_id ~= -1 then
                        local dsx, dsy, dsz = GetScale(fb_id)
                        table.insert(feedback_texts, { id = fb_id, life = 0.0, start_x = mx, start_y = my + 20, base_sx = dsx, base_sy = dsy, base_sz = dsz })
                    end
                    DestroyEntity(circ.id); table.remove(active_circles, i); break
                end
            end
        elseif not IsMouseButtonDown(0) then click_was_down = false end

        if current_progress >= MAX_PROGRESS then
            is_spinning = true
            if e_progress_fill then 
                SetPosition(e_progress_fill, UI_WIDTH / 2, BAR_Y_POS, 0.9)
                SetScale(e_progress_fill, BAR_WIDTH_PX, BAR_HEIGHT_PX * 0.8, 1.0) 
            end
            for _, c in ipairs(active_circles) do DestroyEntity(c.id) end; active_circles = {}
            local uvx, uvy = GetMouseUV()
            last_mx, last_my = uvx * UI_WIDTH, UI_HEIGHT - (uvy * UI_HEIGHT)
            spin_progress = 0.0
            spinner_visual_id = SpawnPrefab(PREFAB_SPINNER, UI_WIDTH / 2, UI_HEIGHT / 2, 0.5)
            SetScale(spinner_visual_id, 200, 200, 1)
            last_angle = math.atan2(100, 0)
        end

    -- 4. PHASE 2: SPINNING
    else
        if is_finished then
            finish_timer = finish_timer - dt
            if finish_timer <= 0 then 
                Mortar.StopMinigame(my_entity_id, true) 
            end
        else
            local uvx, uvy = GetMouseUV()
            local mx, my = uvx * UI_WIDTH, UI_HEIGHT - (uvy * UI_HEIGHT)
            
            if IsMouseButtonDown(0) then
                spoon_x, spoon_y = spoon_x + (mx - last_mx), spoon_y + (my - last_my)
                local len = math.sqrt(spoon_x^2 + spoon_y^2)
                if len > 0.1 then spoon_x, spoon_y = (spoon_x/len)*100, (spoon_y/len)*100 end
                local cur_ang = math.atan2(spoon_y, spoon_x)
                local delta = cur_ang - last_angle
                if delta > math.pi then delta = delta - 2*math.pi 
                elseif delta < -math.pi then delta = delta + 2*math.pi end
                spin_progress, last_angle = spin_progress + math.abs(delta), cur_ang
                
                if spinner_visual_id then SetRotation(spinner_visual_id, 0, 0, math.deg(cur_ang)) end
                if math.abs(delta) > 0.01 then PlayGrinderLoop() else StopGrinderLoop() end
                if spin_progress >= SPIN_REQUIREMENT then 
                    is_finished = true
                    finish_timer = FINISH_DELAY 
                    StopGrinderLoop() 
                end
            else
                StopGrinderLoop()
            end
            last_mx, last_my = mx, my
        end
    end

    -- 5. FEEDBACK CLEANUP
    for i = #feedback_texts, 1, -1 do
        local f = feedback_texts[i]
        f.life = f.life + dt
        if f.life >= 1.0 then 
            DestroyEntity(f.id); table.remove(feedback_texts, i)
        else
            SetPosition(f.id, f.start_x, f.start_y + (f.life * 100), 0.6)
            local s = math.max(1.0 - f.life, 0.01)
            SetScale(f.id, f.base_sx * s, f.base_sy * s, f.base_sz * s)
        end
    end
end

-- --- STOP MINIGAME ---
function Mortar.StopMinigame(targetID, success)
    is_playing = false
    is_camera_flying = false
    is_spinning = false
    is_finished = false
    
    -- Ensure the audio is aggressively cut off if they back out of the minigame early
    StopGrinderLoop()

    -- 1. RESET INTERACTION TEXT
    SetInteractionText(my_entity_id, "Requires Ingredient")
    
    -- 2. Return Player to Original Location
    if start_cam_pos and start_cam_rot then
        TeleportPlayer(start_cam_pos.x, start_cam_pos.y, start_cam_pos.z, 0, start_cam_rot.yaw, 0)
        SetCameraRot(start_cam_rot.pitch, start_cam_rot.yaw)
    end
    
    SetUIFocused(false)
    SetCameraLock(false)
    
    -- 3. Cleanup UI Entities
    if e_progress_frame then DestroyEntity(e_progress_frame); e_progress_frame = nil end
    if e_progress_fill then DestroyEntity(e_progress_fill); e_progress_fill = nil end
    if spinner_visual_id then DestroyEntity(spinner_visual_id); spinner_visual_id = nil end
    
    for _, c in ipairs(active_circles) do DestroyEntity(c.id) end
    active_circles = {}
    
    -- 4. Handle Crafting Success
    if success and current_recipe_result_name ~= "" then
        stored_ingredients = {}
        for _, v in ipairs(visual_entities) do 
            if v ~= -1 then DestroyEntity(v) end 
        end
        visual_entities = {}
        
        local prefabName = current_recipe_result_name .. "_Prefab"
        -- Spawning at the collection coordinates
        SpawnPrefab(prefabName, -47, 7.4, 1.2) 
        Sound.Play("event:/RecipeComplete")
    end
end

-- --- INTERACT ---
function Mortar.OnInteract(senderID, targetID, actionType)
    local id_to_use = my_entity_id or targetID
    
    if is_playing then 
        Mortar.StopMinigame(id_to_use, false)
        return 
    end
    
    local held_item_id = GetSelectedItem()

    -- [BRANCH A] ADDING INGREDIENTS
    if held_item_id ~= -1 then
        if #stored_ingredients < MAX_INGREDIENTS then
            table.insert(stored_ingredients, held_item_id)
            ConsumeSelectedItem()
            
            local vid = -1
            if SpawnCauldronItem then 
                vid = SpawnCauldronItem(id_to_use, held_item_id) 
            end
            table.insert(visual_entities, vid)
            
            if vid ~= -1 and SetRotation then
                SetRotation(vid, ROT_X, ROT_Y, ROT_Z) 
            end
            Sound.Play("event:/PlaceDownItem")
            
            -- Update text prompt to show we have items
            SetInteractionText(id_to_use, "Grind Ingredients")
        else
            print("[Mortar] Bowl is full!")
        end

    -- [BRANCH B] STARTING THE PROCESS
    elseif #stored_ingredients > 0 then
        local resID = CheckValidRecipe(stored_ingredients)
        
        if resID ~= -1 then
            current_recipe_result_name = GetItemNameById(resID)
            local player_id = FindEntityByName("Player")
            
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
                is_camera_flying = true
                camera_flight_t = 0.0
            end

            is_playing = true
            is_spinning = false
            is_finished = false
            current_progress = 0 
            click_was_down = false 
            
            e_progress_frame = SpawnPrefab(PREFAB_FRAME, UI_WIDTH / 2, BAR_Y_POS, 0.8)
            e_progress_fill  = SpawnPrefab(PREFAB_FILL, UI_WIDTH / 2, BAR_Y_POS, 0.9)
            
            if e_progress_frame and e_progress_fill then
                SetIsUI(e_progress_frame, true)
                SetIsUI(e_progress_fill, true)
                SetScale(e_progress_frame, BAR_WIDTH_PX, BAR_HEIGHT_PX, 1.0)
                SetScale(e_progress_fill, 0, BAR_HEIGHT_PX * 0.8, 1.0) 
                SetPosition(e_progress_frame, UI_WIDTH / 2, BAR_Y_POS, 0.8)
                SetPosition(e_progress_fill, (UI_WIDTH / 2) - (BAR_WIDTH_PX / 2), BAR_Y_POS, 0.9)
            end

            SetInteractionText(id_to_use, "Grinding...") 
        else
            -- FAIL: Reset result name so nothing spawns on accidental clicks
            current_recipe_result_name = ""
            Sound.Play("event:/InvalidRecipe")
            print("[Mortar] Invalid recipe.")
        end
    else
        SetInteractionText(id_to_use, "Requires Ingredient")
    end
end

-- DEFINE THE FUNCTION WITHOUT THE 'Mortar.' PREFIX SO IT IS GLOBAL
function MortarReset()
    print("[Mortar] --- RESET TRIGGERED ---")
    -- Since this is now global, use 'Mortar.StopMinigame' to reference the table
    if is_playing then 
        Mortar.StopMinigame(my_entity_id, false) 
    end
    
    for i, vis_id in pairs(visual_entities) do
        if vis_id and DestroyEntity then DestroyEntity(vis_id) end
    end
    
    stored_ingredients = {}
    visual_entities = {}
    is_playing = false
    current_progress = 0
    
    if my_entity_id then
        SetInteractionText(my_entity_id, "Requires Ingredient")
    end
end

return Mortar