-- StatsSystem.lua
-- Attach to a persistent entity (e.g. "StatsController").
-- Controls both the per-day screen (tag: "daystats") and the
-- end-of-game journey screen (tag: "endstats").
--
-- KEY BEHAVIOUR:
--   All stats text entities start with screenSpace=false (set in on_init).
--   screenSpace is only enabled when their screen is active, and disabled again
--   when it deactivates. This ensures stats text is NEVER rendered outside
--   its intended engine state.

local DayStatsScreen = {}

local TOTAL_DAYS  = 5
local TOTAL_STARS = TOTAL_DAYS * 3   -- 15 for End Stats Screen

local PREFAB_STAR = "star_Prefab"

-- ── Star sizes ────────────────────────────────────────────────
local DAY_STAR_SCALE     = 60    -- per-day screen large stars
local ROW_STAR_SCALE     = 20    -- end screen row stars (per-day column)
local GRID_STAR_SCALE    = 40    -- end screen total grid stars

-- ── Text colours (parchment-friendly) ────────────────────────
local COLOR_VALUE    = { r = 0.25, g = 0.13, b = 0.05 }  -- dark brown  (numbers)
local COLOR_LABEL    = { r = 0.35, g = 0.20, b = 0.08 }  -- mid brown   (labels/quote)
local COLOR_PERF     = { r = 0.55, g = 0.32, b = 0.02 }  -- deep amber  (performance/title)

-- ── Fade-in settings ──────────────────────────────────────────
local FADE_DURATION       = 0.9     -- seconds per text element 0→1
local STAGGER_DELAY       = 0.3     -- seconds between each text element AND each star spawn

-- ── Star scale-in settings ────────────────────────────────────
local STAR_SCALE_DURATION = 0.5     -- seconds for each star to scale 0→full size
local STAR_OVERSHOOT      = 3.0     -- overshoot amount: higher = more bounce (try 1.0–3.0)

-- ── Button scale-in settings ────────────────────────────────
local BTN_SCALE_DURATION = 0.4     -- seconds for button to scale 0→full size

-- ── Star audio ───────────────────────────────────────────────
local STAR_SOUNDS = {
    [0] = "event:/StatsTwinkle1",
    [1] = "event:/StatsTwinkle2",
    [2] = "event:/StatsTwinkle3",
}
local starAudioIndex = 0   -- cycles 0,1,2 → wraps back to 0 after every 3 stars

-- ── Fade state ────────────────────────────────────────────────
-- Each entry: { name=string, startTime=number }
local fadeItems    = {}
local staggerIndex = 0   -- reset at the start of each populate call

-- ── Pending star spawns ───────────────────────────────────────
-- Each entry: { x, y, z, scale, list, spawnTime }
local pendingStars = {}

-- ── Active star scale-ins ─────────────────────────────────────
-- Each entry: { entity, targetScale, startTime }
local scalingStars = {}

-- ── Pending button scale-ins ─────────────────────────────────
-- Each entry: { name=string, scaleX, scaleY, startTime }
local pendingButtons = {}

-- ── Active button scale-ins ──────────────────────────────────
-- Each entry: { entity, scaleX, scaleY, startTime }
local scalingButtons = {}

-- ── Populate flags ────────────────────────────────────────────
local dayScreenPopulated = false
local endScreenPopulated = false

-- ── Spawned star tracking ─────────────────────────────────────
local spawnedDayStars   = {}
local spawnedRowStars   = {}
local spawnedTotalStars = {}

-- ── All text entity names for each screen ─────────────────────
local DAY_TEXT_NAMES = {
    "Stats_DayLabel",
    "Stats_GoldFromCustomers",
    "Stats_TimeBonusGold",
    "Stats_TotalGold",
    "Stats_CustomersServed",
    "Stats_TimeRemaining",
    "Stats_PerformanceLabel",
    "Stats_Quote",
}

local END_TEXT_NAMES = {
    "EndStats_Row1Gold",   "EndStats_Row1Served",
    "EndStats_Row2Gold",   "EndStats_Row2Served",
    "EndStats_Row3Gold",   "EndStats_Row3Served",
    "EndStats_Row4Gold",   "EndStats_Row4Served",
    "EndStats_Row5Gold",   "EndStats_Row5Served",
    "EndStats_TotalGold",
    "EndStats_TotalServed",
    "EndStats_Title",
    "EndStats_Quote",
}

-- ── Star rating labels and quotes (keyed 0-3) ─────────────────
local STAR_LABELS = {
    [0] = "No Stars",
    [1] = "Getting Started",
    [2] = "Decent Work",
    [3] = "Outstanding!"
}
local STAR_QUOTES = {
    [0] = '"Even the greatest alchemists had rough days. Tomorrow is another chance."',
    [1] = '"A steady hand learns from each brew. Keep at it."',
    [2] = '"A steady hand and a sturdy cauldron - not bad for a day\'s work."',
    [3] = '"The remedies flowed like a river today. The village is grateful."'
}

-- ── End-screen title / quote ──────────────────────────────────
local function GetEndTitle(total)
    if total >= 15 then return "Legendary Alchemist"
    elseif total >= 12 then return "Renowned Brewer"
    elseif total >= 9  then return "Skilled Potion Maker"
    elseif total >= 5  then return "Apprentice Brewer"
    else return "Novice Alchemist" end
end

local function GetEndQuote(total)
    if total >= 15 then
        return '"Every remedy was perfect. The land will sing of your craft for generations."'
    elseif total >= 12 then
        return '"Through forest paths and quarry dust, your remedies brought comfort to all who sought them. The land will remember your craft."'
    elseif total >= 9 then
        return '"A commendable journey. Many were helped by your brews."'
    elseif total >= 5 then
        return '"You learned much on this road. Return stronger next time."'
    else
        return '"The cauldron bubbled, but not always with success. There is more to learn."'
    end
end

-- ── Helpers ───────────────────────────────────────────────────
local function FormatTime(seconds)
    local s = math.floor(seconds)
    local m = math.floor(s / 60)
    return string.format("%d:%02d", m, s % 60)
end

-- Back ease-out curve: grows past 1.0 then settles back to exactly 1.0.
-- t should be 0→1. Uses STAR_OVERSHOOT constant for bounce amount.
local function EaseOutBack(t)
    local s = STAR_OVERSHOOT
    t = t - 1
    return t * t * ((s + 1) * t + s) + 1
end

-- Enable screenSpace + set text + opacity 0, register for fade-in using staggerIndex
local function SafeSetText(name, value, color)
    local e = FindEntityByName(name)
    if not e then return end
    SetTextComponent(e, "screenSpace", true)
    SetTextComponent(e, "text",        tostring(value))
    SetTextComponent(e, "opacity",     0.0)
    if color then
        SetTextComponent(e, "color", color)
    end
    table.insert(fadeItems, {
        name      = name,
        startTime = GetRealTime() + staggerIndex * STAGGER_DELAY,
    })
    staggerIndex = staggerIndex + 1
end

-- Same as SafeSetText but uses an explicit startTime instead of staggerIndex.
-- Used for elements that must wait for stars to finish before appearing.
local function SafeSetTextAt(name, value, color, startTime)
    local e = FindEntityByName(name)
    if not e then return end
    SetTextComponent(e, "screenSpace", true)
    SetTextComponent(e, "text",        tostring(value))
    SetTextComponent(e, "opacity",     0.0)
    if color then
        SetTextComponent(e, "color", color)
    end
    table.insert(fadeItems, {
        name      = name,
        startTime = startTime,
    })
end

-- Disable screenSpace + zero opacity (completely removes from UI text pass)
local function HideText(name)
    local e = FindEntityByName(name)
    if not e then return end
    SetTextComponent(e, "opacity",     0.0)
    SetTextComponent(e, "screenSpace", false)
end

-- Disable screenSpace on a list of entity names
local function HideTextList(names)
    for _, name in ipairs(names) do
        HideText(name)
    end
end

local function ClearEntities(list)
    for _, e in ipairs(list) do
        if e then DestroyEntity(e) end
    end
end

-- Finds entity by name, sets its scale to 0, queues it for bounce scale-in.
local function QueueButton(name, scaleX, scaleY, startTime)
    local e = FindEntityByName(name)
    if not e then return end
    SetScale(e, 0, 0, 1)
    table.insert(pendingButtons, {
        entity    = e,
        scaleX    = scaleX,
        scaleY    = scaleY,
        startTime = startTime,
    })
end

-- ── Generic star queue helper ─────────────────────────────────
-- Queues `count` stars into pendingStars starting at firstSpawnTime.
-- positions: array of { x, y, z }
-- scale: target scale for all stars in this group
-- list: the tracking table to insert spawned entities into (e.g. spawnedRowStars)
-- Returns the absolute time when the last star finishes its scale-in.
local function QueueStars(count, positions, scale, list, firstSpawnTime)
    for i = 1, count do
        local pos = positions[i]
        table.insert(pendingStars, {
            x         = pos.x,
            y         = pos.y,
            z         = pos.z,
            scale     = scale,
            list      = list,
            spawnTime = firstSpawnTime + (i - 1) * STAGGER_DELAY,
        })
    end
    local lastSpawnTime = firstSpawnTime + (count - 1) * STAGGER_DELAY
    return lastSpawnTime + STAR_SCALE_DURATION
end

-- ═════════════════════════════════════════════════════════════
-- DAY SCREEN STARS
-- 3 large stars under "Today's Performance"
-- PLACEHOLDER positions — tune X/Y once visible in game
-- ═════════════════════════════════════════════════════════════
-- ── Day screen background textures (swap paths when assets are ready) ──
local DAY_BG_TEXTURES = {
    [0] = "Assets/Textures/OneStarEndDay.png",    -- 0 stars
    [1] = "Assets/Textures/OneStarEndDay.png",    -- 1 star
    [2] = "Assets/Textures/TwoStarEndDay.png",    -- 2 stars
    [3] = "Assets/Textures/ThreeStarEndDay.png",  -- 3 stars
}

local DAY_STAR_POSITIONS = {
    { x = 880,  y = 383, z = 0.6 },   -- Star 1 (left)
    { x = 960,  y = 383, z = 0.6 },   -- Star 2 (center)
    { x = 1040, y = 383, z = 0.6 },   -- Star 3 (right)
}

-- Queues day stars and returns time when last star finishes scale-in.
local function QueueDayStars(stars, firstSpawnTime)
    ClearEntities(spawnedDayStars)
    for k in pairs(spawnedDayStars) do spawnedDayStars[k] = nil end  -- wipe in place, keep same table ref
    return QueueStars(stars, DAY_STAR_POSITIONS, DAY_STAR_SCALE, spawnedDayStars, firstSpawnTime)
end

-- ═════════════════════════════════════════════════════════════
-- END SCREEN — ROW STARS
-- 3 small stars per day row, staggered after that row's text
-- PLACEHOLDER positions — tune once visible in game
-- ═════════════════════════════════════════════════════════════
local ROW_STAR_START_X   = 1213
local ROW_STAR_SPACING_X = 22
local ROW_STAR_START_Y   = 825
local ROW_STAR_SPACING_Y = -57   -- negative = moves up each row
local ROW_STAR_Z         = 1.0

-- Builds the position list for a single row's stars.
local function BuildRowStarPositions(rowIndex, starCount)
    local positions = {}
    local y = ROW_STAR_START_Y + (rowIndex - 1) * ROW_STAR_SPACING_Y
    if rowIndex == 5 then y = 591 end   -- explicit override: row 5 margin differs
    for s = 1, starCount do
        table.insert(positions, {
            x = ROW_STAR_START_X + (s - 1) * ROW_STAR_SPACING_X,
            y = y,
            z = ROW_STAR_Z,
        })
    end
    return positions
end

-- ═════════════════════════════════════════════════════════════
-- END SCREEN — TOTAL STAR GRID
-- Up to 15 small stars in a 5×3 grid, queued one by one
-- PLACEHOLDER positions — tune once visible in game
-- ═════════════════════════════════════════════════════════════
local GRID_COLS      = 5
local GRID_START_X   = 866
local GRID_SPACING_X = 50
local GRID_START_Y   = 378
local GRID_SPACING_Y = -45   -- negative = moves up each row
local GRID_Z         = 1.0

-- Builds the position list for the total star grid.
local function BuildGridStarPositions(totalStars)
    local positions = {}
    for i = 1, totalStars do
        local col = (i - 1) % GRID_COLS
        local row = math.floor((i - 1) / GRID_COLS)
        table.insert(positions, {
            x = GRID_START_X + col * GRID_SPACING_X,
            y = GRID_START_Y + row * GRID_SPACING_Y,
            z = GRID_Z,
        })
    end
    return positions
end

-- Queues all grid stars at the SAME spawnTime (simultaneous bounce-in).
-- Plays StatsTwinkle3 once for the whole group via gridGroupSfx flag.
-- Returns the time when the scale-in finishes.
local function QueueGridStars(totalStars, firstSpawnTime)
    local positions = BuildGridStarPositions(totalStars)
    for i = 1, totalStars do
        local pos = positions[i]
        table.insert(pendingStars, {
            x            = pos.x,
            y            = pos.y,
            z            = pos.z,
            scale        = GRID_STAR_SCALE,
            list         = spawnedTotalStars,
            spawnTime    = firstSpawnTime,   -- all same time = simultaneous
            gridGroupSfx = (i == 1),         -- only first entry triggers the sound
        })
    end
    return firstSpawnTime + STAR_SCALE_DURATION
end

-- ═════════════════════════════════════════════════════════════
-- PER-DAY SCREEN POPULATE
-- Reveal order:
--   DayLabel → GoldFromCustomers → TimeBonusGold →
--   CustomersServed → TimeRemaining → TotalGold →
--   ⭐ stars one by one (bounce scale-in) →
--   PerformanceLabel → Quote
-- ═════════════════════════════════════════════════════════════
function DayStatsScreen.PopulateDayScreen()
    staggerIndex   = 0
    fadeItems      = {}
    pendingStars   = {}
    scalingStars   = {}
    pendingButtons = {}
    scalingButtons = {}
    starAudioIndex = 0
    
    local currentDayIdx = GetCurrentDay()
    local history = GetAllDayStats()
    local d = nil
-- SEARCH the array for the entry where dayIndex matches the current day
    if history then
        for _, entry in ipairs(history) do
            if entry and entry.dayIndex == currentDayIdx then
                d = entry
                break
            end
        end
    end

    -- Fallback if nothing was found
    if not d then
        print("[StatsSystem] ERROR: No stats found for Day " .. tostring(currentDayIdx))
        d = { dayIndex = currentDayIdx, goldFromCustomers = 0, timeBonusGold = 0, 
              customersServed = 0, timeRemaining = 0, totalGold = 0, stars = 0 }
    end

    -- ── Swap background image based on stars earned ───────────
    local bg = FindEntityByName("DayStats_Base")
    if bg then SetTexture(bg, DAY_BG_TEXTURES[d.stars] or "dayStats_base") end

    -- ── Text before stars (indices 0–5) ───────────────────────
    SafeSetText("Stats_DayLabel", "Day " .. d.dayIndex .. " of " .. TOTAL_DAYS, COLOR_LABEL)
    SafeSetText("Stats_GoldFromCustomers", tostring(d.goldFromCustomers),                   COLOR_VALUE)
    SafeSetText("Stats_TimeBonusGold",     "+ " .. d.timeBonusGold .. " G day time bonus",  COLOR_LABEL)
    SafeSetText("Stats_CustomersServed",   tostring(d.customersServed),                     COLOR_VALUE)
    SafeSetText("Stats_TimeRemaining",     FormatTime(d.timeRemaining),                     COLOR_VALUE)
    SafeSetText("Stats_TotalGold",         d.totalGold .. " G",                             COLOR_VALUE)
    -- staggerIndex is now 6 after the above six calls

    -- ── Stars: spawn after TotalGold fully fades in ────────────
    local totalGoldFinishTime = GetRealTime() + 5 * STAGGER_DELAY + FADE_DURATION
    local allStarsDoneTime    = QueueDayStars(d.stars, totalGoldFinishTime)

    -- ── Text after stars ───────────────────────────────────────
    local afterStars = allStarsDoneTime + STAGGER_DELAY
    SafeSetTextAt("Stats_PerformanceLabel", STAR_LABELS[d.stars] or "Decent Work", COLOR_PERF,  afterStars)
    SafeSetTextAt("Stats_Quote",            STAR_QUOTES[d.stars] or "",            COLOR_LABEL, afterStars + STAGGER_DELAY)

    -- ── Continue button: appears after Quote fades in ─────────
    local continueTime = afterStars + STAGGER_DELAY + FADE_DURATION
    QueueButton("DayStats_ContinueBtn", 150, 60, continueTime)

    print("[StatsSystem] Day " .. d.dayIndex .. " screen populated. Stars: " .. d.stars)
end

-- ═════════════════════════════════════════════════════════════
-- END-OF-GAME SCREEN POPULATE
-- Reveal order per row:
--   Row N Gold + Served → ⭐ row N stars (one by one)
-- Then after all rows:
--   TotalGold + TotalServed → ⭐ total star grid (one by one) →
--   Title → Quote
-- ═════════════════════════════════════════════════════════════
function DayStatsScreen.PopulateEndScreen()
staggerIndex = 0
    fadeItems = {}
    pendingStars = {}
    scalingStars = {}
    pendingButtons = {}
    scalingButtons = {}
    starAudioIndex = 0
    
    ClearEntities(spawnedRowStars)
    for k in pairs(spawnedRowStars)   do spawnedRowStars[k]   = nil end
    ClearEntities(spawnedTotalStars)
    for k in pairs(spawnedTotalStars) do spawnedTotalStars[k] = nil end

    local history = GetAllDayStats() or {}
    local totalGold, totalServed, totalStars = 0, 0, 0
    local now    = GetRealTime()
    local cursor = now 

    -- LOOP 1 to 5 (Rows on the screen)
    for i = 1, TOTAL_DAYS do
        local dayData = nil
        
        -- SEARCH the "stack" to see if any entry belongs to Day 'i'
        for _, entry in ipairs(history) do
            if entry and entry.dayIndex == i then
                dayData = entry
                break
            end
        end

        local displayGold = 0
        local displayServed = 0
        local stars = 0

        if dayData then
            displayGold   = dayData.totalGold or 0
            displayServed = dayData.customersServed or 0
            stars         = dayData.stars or 0
            
            -- Accumulate Grand Totals
            totalGold   = totalGold + displayGold
            totalServed = totalServed + displayServed
            totalStars  = totalStars + stars
        end

        -- Populate UI for Row 'i'
        SafeSetTextAt("EndStats_Row" .. i .. "Gold",   tostring(displayGold), COLOR_VALUE, cursor)
        cursor = cursor + STAGGER_DELAY
        SafeSetTextAt("EndStats_Row" .. i .. "Served", tostring(displayServed), COLOR_VALUE, cursor)
        cursor = cursor + STAGGER_DELAY

        -- Star spawning for Row 'i'
        if stars > 0 then
            local rowStarStart = cursor + FADE_DURATION
            local positions    = BuildRowStarPositions(i, stars)
            starAudioIndex     = 0 
            local rowDoneTime  = QueueStars(stars, positions, ROW_STAR_SCALE, spawnedRowStars, rowStarStart)
            cursor = rowDoneTime + STAGGER_DELAY
        else
            cursor = cursor + FADE_DURATION + STAGGER_DELAY
        end
    end

    -- ── Totals text after all rows done ───────────────────────
    SafeSetTextAt("EndStats_TotalGold",   string.format("%d G", totalGold), COLOR_VALUE, cursor)
    cursor = cursor + STAGGER_DELAY
    SafeSetTextAt("EndStats_TotalServed", tostring(totalServed),            COLOR_VALUE, cursor)
    cursor = cursor + STAGGER_DELAY

    -- ── Total star grid after TotalServed fades in ────────────
    local gridStarStart = cursor + FADE_DURATION
    local gridDoneTime  = QueueGridStars(totalStars, gridStarStart)

    -- ── Title + Quote after grid finishes ─────────────────────
    local afterGrid = gridDoneTime + STAGGER_DELAY
    SafeSetTextAt("EndStats_Title", GetEndTitle(totalStars), COLOR_PERF,  afterGrid)
    SafeSetTextAt("EndStats_Quote", GetEndQuote(totalStars), COLOR_LABEL, afterGrid + STAGGER_DELAY)

    -- ── Continue button: appears after Quote fades in ─────────
    local continueTime = afterGrid + STAGGER_DELAY + FADE_DURATION
    QueueButton("EndStats_ContinueBtn", 200, 80, continueTime)

    print("[StatsSystem] End screen populated. Total stars: " .. totalStars .. "/" .. TOTAL_STARS)
end

-- ═════════════════════════════════════════════════════════════
-- ECS HOOKS
-- ═════════════════════════════════════════════════════════════
function DayStatsScreen:on_init()
    print("[StatsSystem] Initialized.")

    -- Disable screenSpace on ALL stats text at startup.
    -- This prevents them from entering the UI text render pass
    -- until their respective screen is actually active.
    HideTextList(DAY_TEXT_NAMES)
    HideTextList(END_TEXT_NAMES)

    dayScreenPopulated = false
    endScreenPopulated = false
    spawnedDayStars    = {}
    spawnedRowStars    = {}
    spawnedTotalStars  = {}
    fadeItems          = {}
    pendingStars       = {}
    scalingStars       = {}
    pendingButtons     = {}
    scalingButtons     = {}
    staggerIndex       = 0
    starAudioIndex     = 0
end

function DayStatsScreen:on_update(dt)
    local now = GetRealTime()

    -- ── Fade tick ──────────────────────────────────────────────
    for i = #fadeItems, 1, -1 do
        local item = fadeItems[i]
        if now >= item.startTime then
            local t      = math.min((now - item.startTime) / FADE_DURATION, 1.0)
            local smooth = t * t * (3 - 2 * t)   -- smoothstep ease-in-out
            local e = FindEntityByName(item.name)
            if e then SetTextComponent(e, "opacity", smooth) end
            if t >= 1.0 then table.remove(fadeItems, i) end
        end
    end

    -- ── Pending star spawn tick ────────────────────────────────
    -- Spawns each star at scale 0 then hands it to scalingStars
    local gridSfxPlayed = false   -- ensures StatsTwinkle3 fires only once per frame
    for i = #pendingStars, 1, -1 do
        local s = pendingStars[i]
        if now >= s.spawnTime then
            local e = SpawnPrefab(PREFAB_STAR, s.x, s.y, s.z)
            if e then
                SetScale(e, 0, 0, 1)   -- start at zero; scale tick grows it
                if s.gridGroupSfx then
                    if not gridSfxPlayed then
                        Sound.Play("event:/StatsTwinkle3")  -- one sound for all grid stars
                        gridSfxPlayed = true
                    end
                else
                    Sound.Play(STAR_SOUNDS[starAudioIndex % 3])
                    starAudioIndex = starAudioIndex + 1
                end
                table.insert(s.list, e)
                table.insert(scalingStars, {
                    entity      = e,
                    targetScale = s.scale,
                    startTime   = now,
                })
            end
            table.remove(pendingStars, i)
        end
    end

    -- ── Star scale-in tick (overshoot bounce) ─────────────────
    for i = #scalingStars, 1, -1 do
        local s = scalingStars[i]
        local t = math.min((now - s.startTime) / STAR_SCALE_DURATION, 1.0)
        local scale = s.targetScale * EaseOutBack(t)
        SetScale(s.entity, scale, scale, 1)
        if t >= 1.0 then table.remove(scalingStars, i) end
    end

    -- ── Pending button scale-in tick ───────────────────────────
    for i = #pendingButtons, 1, -1 do
        local b = pendingButtons[i]
        if now >= b.startTime then
            table.insert(scalingButtons, {
                entity    = b.entity,
                scaleX    = b.scaleX,
                scaleY    = b.scaleY,
                startTime = now,
            })
            table.remove(pendingButtons, i)
        end
    end

    -- ── Active button scale-in tick (overshoot bounce) ─────────
    for i = #scalingButtons, 1, -1 do
        local b = scalingButtons[i]
        local t = math.min((now - b.startTime) / BTN_SCALE_DURATION, 1.0)
        local ease = EaseOutBack(t)
        SetScale(b.entity, b.scaleX * ease, b.scaleY * ease, 1)
        if t >= 1.0 then table.remove(scalingButtons, i) end
    end

    -- ── Per-day screen ─────────────────────────────────────────
    if IsEngineState("DayStats") then
        if not dayScreenPopulated then
            DayStatsScreen.PopulateDayScreen()
            dayScreenPopulated = true
        end
    else
        if dayScreenPopulated then
            HideTextList(DAY_TEXT_NAMES)
            ClearEntities(spawnedDayStars)
            spawnedDayStars = {}
            fadeItems       = {}
            pendingStars    = {}
            scalingStars    = {}
            pendingButtons  = {}
            scalingButtons  = {}
        end
        dayScreenPopulated = false
    end

    -- ── End screen ─────────────────────────────────────────────
    if IsEngineState("EndStats") then
        if not endScreenPopulated then
            DayStatsScreen.PopulateEndScreen()
            endScreenPopulated = true
        end
    else
        if endScreenPopulated then
            HideTextList(END_TEXT_NAMES)
            ClearEntities(spawnedRowStars)
            ClearEntities(spawnedTotalStars)
            spawnedRowStars   = {}
            spawnedTotalStars = {}
            fadeItems         = {}
            pendingStars      = {}
            scalingStars      = {}
            pendingButtons    = {}
            scalingButtons    = {}
        end
        endScreenPopulated = false
    end
end

return DayStatsScreen