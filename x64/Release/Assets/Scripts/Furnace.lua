local M = {}

-- ============================================================
-- [CONFIGURATION]
-- ============================================================

-- ── RECIPE CONFIGURATION ──────────────────────────────────────
local MAX_INGREDIENTS = 2 -- Maximum number of items the furnace can hold

-- ── SCREEN PLACEMENT ──────────────────────────────────────────
local SCREEN_POS_X = 0.65      -- 75% across = right side of screen
local SCREEN_POS_Y = 0.40      -- 50% up     = vertically centred
local UI_SCALE     = 0.55

-- ── BAR DIMENSIONS ──
local BAR_HEIGHT   = 0.7
local BAR_WIDTH    = 0.14
local CATCH_HEIGHT = 0.15
local FISH_SIZE    = 0.05
local HEAT_WIDTH   = 0.055
local BAR_GAP      = 0.05
local FLAT_Z       = 0.008

local PREFAB_TRACK = "ProgressBarFrame_Prefab"
local PREFAB_FILL  = "ProgressBarFill_Prefab"

-- ── ANIMATION NAMES ──────────────────────────────────────────
local ANIM_BELLOWS = "Bellows"
local ANIM_FURNACE = "Furnance"

-- ── TEXTURES ─────────────────────────────────────────────────
local TEX_BG           = "Assets/Textures/ui_background.png"
local TEX_FRAME        = "Assets/Textures/track_frame.png"
local TEX_CATCH_IDLE   = "Assets/Textures/catch_idle.png"
local TEX_CATCH_ACTIVE = "Assets/Textures/catch_active.png"
local TEX_CATCH_GRACE  = "Assets/Textures/catch_grace.png"
local TEX_FISH         = "Assets/Textures/fish_ember.png"
local TEX_PROG = {
    "Assets/Textures/progress_cold.png",
    "Assets/Textures/progress_warm.png",
    "Assets/Textures/progress_hot.png",
    "Assets/Textures/progress_whitehot.png",
}

-- ── PLAYER TELEPORT ───────────────────────────────────────────
local FURNACE_FACING      = 0
local PLAYER_DIST         = 3.0
local PLAYER_OFFSET_RIGHT = 0.0
local PLAYER_Y_OFFSET     = -2.0

-- ── PHYSICS & SPAWN MATH ──────────────────────────────────────
local CATCH_GRAVITY      = 2.8   
local CATCH_LIFT         = 4.0   
local CATCH_MAX_VEL      = 2.8   
local CATCH_BOUNCE       = 0.05  
local FISH_SPEED_MIN     = 0.6
local FISH_SPEED_MAX     = 2.6
local FISH_IDLE_MIN      = 0.6
local FISH_IDLE_MAX      = 1.8
local PROGRESS_GAIN      = 0.35
local PROGRESS_DRAIN     = 0.10
local WIN_THRESHOLD      = 1.0
local START_PROGRESS     = 0.15

-- SPAWN OFFSET LOGIC:
local SPAWN_Y_OFFSET     = 0.5 
local SPAWN_FORWARD_DIST = 1.5

-- ============================================================
-- [STATE]
-- ============================================================
local my_entity_id = nil
local bellows_id   = nil
local stored_ingredients = {} 
local visual_entities    = {}    
local result_item_id     = nil

local is_active        = false
local was_mouse_down   = false 

-- Memory for the Bellows original transform
local orig_bellows_pos = {x = 0, y = 0, z = 0}
local orig_bellows_rot = {x = 0, y = 0, z = 0}
local orig_bellows_scl = {x = 1, y = 1, z = 1} -- New scale memory

local progress, catch_pos, catch_vel = 0.0, 0.0, 0.0
local fish_pos, fish_target, fish_speed, fish_timer = 0.2, 0.2, 0.0, 0.0
local grace_timer  = 0.0
local cur_catch_tex = ""
local cur_prog_tex  = ""

local e_background = nil
local e_bar_frame  = nil
local e_heat_frame = nil
local e_catch      = nil
local e_fish       = nil
local e_heat_fill  = nil

-- ============================================================
-- HELPERS
-- ============================================================
local function Clamp(v, lo, hi) return math.max(lo, math.min(v, hi)) end
local function RandRange(lo, hi) return lo + math.random()*(hi-lo) end
local function Sign(v)
    if v > 0 then return 1 elseif v < 0 then return -1 else return 0 end
end
local function SwapTex(e, new, cur)
    if new ~= cur then SetTexture(e, new) end
    return new
end
local function GetProgTex(p)
    local t = p / WIN_THRESHOLD
    if     t < 0.25 then return TEX_PROG[1]
    elseif t < 0.50 then return TEX_PROG[2]
    elseif t < 0.75 then return TEX_PROG[3]
    else                  return TEX_PROG[4] end
end

-- ============================================================
-- INIT
-- ============================================================
function M.on_init(self)
    my_entity_id = self.entity
    math.randomseed(os.time())
    bellows_id = FindEntityByName("Bellows")
    SubscribeToEvent("ResetWorkstations", "Furnace_ResetStation", my_entity_id)
end

-- ============================================================
-- EJECT BAD INGREDIENTS
-- ============================================================
function M.EjectItems()
    print("[Furnace] Invalid recipe! Ejecting items...")
    
    for _, vis_id in ipairs(visual_entities) do
        if vis_id ~= -1 then DestroyEntity(vis_id) end
    end
    
    local cx, cy, cz = GetPosition(my_entity_id)
    local rx, ry, rz = GetRotation(my_entity_id)
    
    if cx then
        local fr      = math.rad(ry + FURNACE_FACING)
        local fwd_x   = math.cos(fr)
        local fwd_z   = math.sin(fr)
        
        local spawn_x = cx + (fwd_x * SPAWN_FORWARD_DIST)
        local spawn_y = cy + SPAWN_Y_OFFSET
        local spawn_z = cz + (fwd_z * SPAWN_FORWARD_DIST)
        
        for _, item_id in ipairs(stored_ingredients) do
            local prefabName = GetItemNameById(item_id) .. "_Prefab"
            SpawnPrefab(prefabName, spawn_x, spawn_y, spawn_z) 
        end
    end
    
    stored_ingredients = {}
    visual_entities = {}
end

-- ============================================================
-- INTERACT LOGIC
-- ============================================================
function M.OnInteract()
    local held_item_id = GetSelectedItem()

    -- [BRANCH A] ADDING INGREDIENT
    if held_item_id ~= -1 then
        if #stored_ingredients >= MAX_INGREDIENTS then
            print("[Furnace] Furnace is already full!")
            return 
        end

        table.insert(stored_ingredients, held_item_id)
        ConsumeSelectedItem()
        
        if Sound and Sound.Play then Sound.Play("event:/PlaceDownItem") end
        
        local vis_id = -1
        if SpawnFurnaceItem and my_entity_id then
            vis_id = SpawnFurnaceItem(my_entity_id, held_item_id)
        end
        table.insert(visual_entities, vis_id)

        local resultID = -1
        if CheckValidRecipe then
            resultID = CheckValidRecipe(stored_ingredients, "Furnace")
        end

        if resultID ~= -1 then
            print("[Furnace] Valid recipe detected! Starting smelting...")
            result_item_id = resultID
            M.StartMinigame()
            
        elseif #stored_ingredients >= MAX_INGREDIENTS then
            M.EjectItems()
        end
        
        return 
    end

    -- [BRANCH B] MANUAL TOGGLE
    if is_active then 
        M.StopMinigame() 
    end
end

-- ============================================================
-- START
-- ============================================================
function M.StartMinigame()
    is_active      = true
    was_mouse_down = false
    progress       = START_PROGRESS
    catch_pos      = BAR_HEIGHT * 0.5 - CATCH_HEIGHT * 0.5
    catch_vel      = 0.0
    fish_pos       = 0.2
    fish_target    = RandRange(0.1, BAR_HEIGHT - 0.1)
    fish_speed     = RandRange(FISH_SPEED_MIN, FISH_SPEED_MAX)
    fish_timer     = RandRange(FISH_IDLE_MIN,  FISH_IDLE_MAX)
    grace_timer    = 1.2
    cur_catch_tex  = ""
    cur_prog_tex   = ""

    SetCameraLock(true)

    -- Play furnace loop sound (3D at furnace position)
    if Sound and Sound.Play3D then
        local fx, fy, fz = GetPosition(my_entity_id)
        Sound.Play3D("event:/FurnaceLoop", fx, fy, fz)
    end
    
    -- ── START ANIMATION ──
    if SetAnimationSettings and PlayEntityAnimation then
        SetAnimationSettings(my_entity_id, 1.0, true, false)
        PlayEntityAnimation(my_entity_id, ANIM_FURNACE)
    end
    
    -- ── RELOCATE BELLOWS ──
    if bellows_id then
        local bx, by, bz = GetPosition(bellows_id)
        local brx, bry, brz = GetRotation(bellows_id)
        local bsx, bsy, bsz = GetScale(bellows_id) -- Get current scale
        
        if bx then
            -- Store original position, rotation, and scale
            orig_bellows_pos.x, orig_bellows_pos.y, orig_bellows_pos.z = bx, by, bz
            orig_bellows_rot.x, orig_bellows_rot.y, orig_bellows_rot.z = brx, bry, brz
            orig_bellows_scl.x, orig_bellows_scl.y, orig_bellows_scl.z = bsx, bsy, bsz
        end
        
        -- Move to minigame view and scale it down
        SetPosition(bellows_id, -48.00, 7.700, 13.200)
        SetRotation(bellows_id, 270.000, 180.000, 30.000)
        SetScale(bellows_id, 0.2, 0.2, 0.2)
    end

    local pid = FindEntityByName("Player")
    if pid and SetLinearVelocity then SetLinearVelocity(pid, 0, 0, 0) end

    -- Teleport player in front of furnace
    local cx, cy, cz = GetPosition(my_entity_id)
    local rx, ry, rz = GetRotation(my_entity_id)
    if pid and cx then
        local fr      = math.rad(ry + FURNACE_FACING)
        local fwd_x   = math.cos(fr);   local fwd_z   = math.sin(fr)
        local rr      = fr + math.pi * 0.5
        local right_x = math.cos(rr);   local right_z = math.sin(rr)
        local tx      = cx + fwd_x * PLAYER_DIST + right_x * PLAYER_OFFSET_RIGHT
        local tz      = cz + fwd_z * PLAYER_DIST + right_z * PLAYER_OFFSET_RIGHT
        local px, py, pz = GetPosition(pid)
        local yaw     = math.deg(math.atan2(-fwd_z, -fwd_x))
        if TeleportPlayer then TeleportPlayer(tx, py + PLAYER_Y_OFFSET, tz, 0, yaw, 0) end
        if SetCameraRot   then SetCameraRot(0.0, yaw) end
    end

    -- Spawn UI
    e_background = SpawnPrefab(PREFAB_FILL,  my_entity_id)
    e_bar_frame  = SpawnPrefab(PREFAB_TRACK, my_entity_id)
    e_heat_frame = SpawnPrefab(PREFAB_TRACK, my_entity_id)
    e_catch      = SpawnPrefab(PREFAB_FILL,  my_entity_id)
    e_fish       = SpawnPrefab(PREFAB_FILL,  my_entity_id)
    e_heat_fill  = SpawnPrefab(PREFAB_FILL,  my_entity_id)

    SetIsUI(e_background, true)
    SetIsUI(e_bar_frame,  true)
    SetIsUI(e_heat_frame, true)
    SetIsUI(e_catch,      true)
    SetIsUI(e_fish,       true)
    SetIsUI(e_heat_fill,  true)

    SetTexture(e_background, TEX_BG)
    SetTexture(e_bar_frame,  TEX_FRAME)
    SetTexture(e_heat_frame, TEX_FRAME)
    SetTexture(e_catch,      TEX_CATCH_GRACE)
    SetTexture(e_fish,       TEX_FISH)
    SetTexture(e_heat_fill,  TEX_PROG[1])
end

-- ============================================================
-- STOP
-- ============================================================
function M.StopMinigame()
    is_active = false
    SetCameraLock(false)

    -- Stop furnace loop sound
    if Sound and Sound.Stop then
        Sound.Stop("event:/FurnaceLoop")
    end
    
    -- ── STOP ANIMATION ──
    if SetAnimationSettings then
        SetAnimationSettings(my_entity_id, 0.0, false, false)
    end
    if StopAnimation then
        StopAnimation(my_entity_id)
    end

    -- ── RESTORE BELLOWS LOCATION & SCALE ──
    if bellows_id and orig_bellows_pos.x ~= 0 then
        SetPosition(bellows_id, orig_bellows_pos.x, orig_bellows_pos.y, orig_bellows_pos.z)
        SetRotation(bellows_id, orig_bellows_rot.x, orig_bellows_rot.y, orig_bellows_rot.z)
        SetScale(bellows_id, orig_bellows_scl.x, orig_bellows_scl.y, orig_bellows_scl.z)
    end

    local pid = FindEntityByName("Player")
    if pid and SetLinearVelocity then SetLinearVelocity(pid, 0, 0, 0) end

    local function Safe(e) if e then DestroyEntity(e) end end
    Safe(e_background)
    Safe(e_bar_frame); Safe(e_heat_frame)
    Safe(e_catch); Safe(e_fish); Safe(e_heat_fill)
    e_background                  = nil
    e_bar_frame, e_heat_frame     = nil, nil
    e_catch, e_fish, e_heat_fill  = nil, nil, nil
end

-- ============================================================
-- UPDATE
-- ============================================================
function M:on_update(dt)
    if not is_active then return end
    if dt > 0.05 then dt = 0.05 end
    if grace_timer > 0 then grace_timer = grace_timer - dt end

    -- 1. CATCH PHYSICS & ANIMATION
    local is_down = IsMouseButtonDown(0)

    if is_down then 
        catch_vel = catch_vel + CATCH_LIFT * dt
        
        if bellows_id and not was_mouse_down and PlayEntityAnimation then
            if SetAnimationSettings then
                SetAnimationSettings(bellows_id, 1.0, false, false)
            end
            PlayEntityAnimation(bellows_id, ANIM_BELLOWS)
        end
    else                         
        catch_vel = catch_vel - CATCH_GRAVITY * dt 
    end
    
    was_mouse_down = is_down 
    
    catch_vel = Clamp(catch_vel, -CATCH_MAX_VEL, CATCH_MAX_VEL)
    local np = catch_pos + catch_vel * dt
    if     np <= 0                       then np = 0;                       catch_vel =  math.abs(catch_vel) * CATCH_BOUNCE
    elseif np >= BAR_HEIGHT-CATCH_HEIGHT then np = BAR_HEIGHT-CATCH_HEIGHT; catch_vel = -math.abs(catch_vel) * CATCH_BOUNCE end
    catch_pos = np

    -- 2. FISH AI
    fish_timer = fish_timer - dt
    if fish_timer <= 0 then
        fish_target = RandRange(0.05, BAR_HEIGHT - 0.05)
        fish_speed  = RandRange(FISH_SPEED_MIN, FISH_SPEED_MAX)
        fish_timer  = RandRange(FISH_IDLE_MIN,  FISH_IDLE_MAX)
    end
    local df = fish_target - fish_pos
    if math.abs(df) > 0.01 then
        local step = Sign(df) * fish_speed * dt
        if math.abs(step) > math.abs(df) then step = df end
        fish_pos = fish_pos + step
    end
    fish_pos = Clamp(fish_pos, 0, BAR_HEIGHT)

    -- 3. PROGRESS
    local inside = fish_pos >= catch_pos and fish_pos <= catch_pos + CATCH_HEIGHT
    if inside             then progress = progress + PROGRESS_GAIN  * dt
    elseif grace_timer<=0 then progress = progress - PROGRESS_DRAIN * dt end
    progress = Clamp(progress, 0.0, WIN_THRESHOLD)

    -- 4. TEXTURE SWAPS
    local ct
    if   grace_timer > 0 then ct = TEX_CATCH_GRACE
    elseif inside        then ct = TEX_CATCH_ACTIVE
    else                      ct = TEX_CATCH_IDLE end
    cur_catch_tex = SwapTex(e_catch,     ct,                   cur_catch_tex)
    cur_prog_tex  = SwapTex(e_heat_fill, GetProgTex(progress), cur_prog_tex)

    -- 5. UI POSITIONING
    local screen_w, screen_h = 1920, 1080
    if GetScreenSize then screen_w, screen_h = GetScreenSize() end

    local PS = screen_h * UI_SCALE
    local UI_BASE_X = screen_w * SCREEN_POS_X
    local UI_BASE_Y = screen_h * SCREEN_POS_Y

    local left_cx  = -(BAR_WIDTH  * 0.5 + BAR_GAP * 0.5) * PS
    local right_cx =  (HEAT_WIDTH * 0.5 + BAR_GAP * 0.5) * PS
    local half_bar_px = (BAR_HEIGHT * 0.5) * PS

    local D_BG    = -1.0
    local D_FRAME =  0.0
    local D_FILL  =  0.02
    local D_FISH  =  0.04

    local BG_W = (BAR_WIDTH + BAR_GAP + HEAT_WIDTH + 0.16) * PS
    local BG_H = (BAR_HEIGHT + 0.18) * PS
    SetScale(e_background, BG_W,                    BG_H,                FLAT_Z)
    SetScale(e_bar_frame,  BAR_WIDTH   * PS,        BAR_HEIGHT * PS,     FLAT_Z)
    SetScale(e_heat_frame, HEAT_WIDTH  * PS,        BAR_HEIGHT * PS,     FLAT_Z)
    SetScale(e_catch,      BAR_WIDTH   * 0.72 * PS, CATCH_HEIGHT * PS,   FLAT_Z)
    SetScale(e_fish,       FISH_SIZE   * PS,        FISH_SIZE   * PS,    FLAT_Z)

    local ph    = math.max(0.01, (progress / WIN_THRESHOLD) * BAR_HEIGHT)
    local ph_px = ph * PS
    SetScale(e_heat_fill, HEAT_WIDTH * 0.72 * PS, ph_px, FLAT_Z)

    local bg_centre_x = UI_BASE_X + (left_cx + right_cx) * 0.5
    SetPosition(e_background, bg_centre_x, UI_BASE_Y, D_BG)
    SetRotation(e_background, 0, 0, 0)

    SetPosition(e_bar_frame,  UI_BASE_X + left_cx,  UI_BASE_Y, D_FRAME)
    SetRotation(e_bar_frame,  0, 0, 0)
    SetPosition(e_heat_frame, UI_BASE_X + right_cx, UI_BASE_Y, D_FRAME)
    SetRotation(e_heat_frame, 0, 0, 0)

    local catch_y_px = ((catch_pos + CATCH_HEIGHT * 0.5) - BAR_HEIGHT * 0.5) * PS
    SetPosition(e_catch, UI_BASE_X + left_cx, UI_BASE_Y + catch_y_px, D_FILL)
    SetRotation(e_catch, 0, 0, 0)

    local fish_y_px = (fish_pos + FISH_SIZE * 0.5 - BAR_HEIGHT * 0.5) * PS
    SetPosition(e_fish, UI_BASE_X + left_cx, UI_BASE_Y + fish_y_px, D_FISH)
    SetRotation(e_fish, 0, 0, 0)

    local heat_y_px = (-BAR_HEIGHT * 0.5 + ph * 0.5) * PS
    SetPosition(e_heat_fill, UI_BASE_X + right_cx, UI_BASE_Y + heat_y_px, D_FILL)
    SetRotation(e_heat_fill, 0, 0, 0)

    -- 6. WIN / LOSE
    if progress >= WIN_THRESHOLD then M.CompleteSmelting()
    end
end

function M.CompleteSmelting()
    M.StopMinigame()
    
    for _, vis_id in ipairs(visual_entities) do
        if vis_id ~= -1 then DestroyEntity(vis_id) end
    end
    stored_ingredients = {}
    visual_entities = {}

    -- ── DYNAMIC ITEM SPAWN (PUSHED FORWARD) ──
    if result_item_id then
        local prefabName = GetItemNameById(result_item_id) .. "_Prefab"
        local cx, cy, cz = GetPosition(my_entity_id)
        local rx, ry, rz = GetRotation(my_entity_id)
        
        if cx then
            local fr      = math.rad(ry + FURNACE_FACING)
            local fwd_x   = math.cos(fr)
            local fwd_z   = math.sin(fr)
            
            local spawn_x = cx + (fwd_x * SPAWN_FORWARD_DIST)
            local spawn_y = cy + SPAWN_Y_OFFSET
            local spawn_z = cz + (fwd_z * SPAWN_FORWARD_DIST)

            SpawnPrefab(prefabName, spawn_x, spawn_y, spawn_z)
        else
            SpawnPrefab(prefabName, -48, 7, 12)
        end

        if Sound and Sound.Play then
            Sound.Play("event:/RecipeComplete")
        end
    end
    
    result_item_id = nil
end

-- The Event Callback
function Furnace_ResetStation()
    if is_active then 
        M.StopMinigame() 
    end
    
    for _, vis_id in ipairs(visual_entities) do
        if vis_id ~= -1 and DestroyEntity then 
            DestroyEntity(vis_id) 
        end
    end
    
    stored_ingredients = {}
    visual_entities = {}
    print("[Furnace] Station Reset for new day.")
end

function M.FailSmelting() M.StopMinigame() end

return M