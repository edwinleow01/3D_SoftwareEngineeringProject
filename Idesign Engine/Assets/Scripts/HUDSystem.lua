-- HUDSystem.lua
local HUDSystem = {}

-- =========================================================
-- CONSTANTS
-- =========================================================
local PHASE_NIGHT = 2
local MAX_SLOTS   = 2

local THRESHOLD_ORANGE = 0.6
local THRESHOLD_RED    = 0.3

local TEX_GREEN  = "Assets/Textures/patiencebar_green.png"
local TEX_ORANGE = "Assets/Textures/patiencebar_orange.png"
local TEX_RED    = "Assets/Textures/patiencebar_red.png"

local BAR_WIDTH_MAX = 350.0
local BAR_HEIGHT    = 25.0
local BAR_CENTER_Y  = 910.0
local BAR_CENTER_X  = { 180.0, 575.0 }
local BAR_LEFT_EDGE = {
    BAR_CENTER_X[1] - (BAR_WIDTH_MAX / 2),
    BAR_CENTER_X[2] - (BAR_WIDTH_MAX / 2),
}

local HIDE_Z = -9999.0

-- Display Z per layer
local Z = {
    panel = 0.1,  -- SlotPanel
    mid   = 0.2,  -- PatienceFrame, PotionSlot, arrowRight, Icon_Fish/Orc
    top   = 0.3,  -- PatienceFill, PotionIcon
    text  = 0.2,  -- ArrivingText, OrderText
}

-- =========================================================
-- STATE
-- =========================================================
local initialized = false

local slotPanels     = {}
local fishIcons      = {}
local orcIcons       = {}
local arrivingTexts  = {}
local orderTexts     = {}
local patienceFrames = {}
local patienceFills  = {}
local potionSlots    = {}
local potionIcons    = {}
local arrowRights    = {}

-- Cached X/Y per entity, captured once at NightInit
local entityPos = {}

local barStates = { "green", "green" }

-- =========================================================
-- POSITION CACHE
-- =========================================================
local function CachePos(entity)
    if not entity then return end
    if entityPos[entity] then return end
    local ok, x, y, _ = pcall(GetPosition, entity)
    if ok then
        entityPos[entity] = { x = x, y = y }
    end
end

-- =========================================================
-- VISIBILITY HELPERS
-- =========================================================
local function ShowAt(entity, z)
    if not entity then return end
    local p = entityPos[entity]
    if p then
        SetTransformComponent(entity, "position", { x = p.x, y = p.y, z = z })
    end
end

local function Hide(entity)
    if not entity then return end
    local p = entityPos[entity]
    if p then
        SetTransformComponent(entity, "position", { x = p.x, y = p.y, z = HIDE_Z })
    end
end

-- =========================================================
-- TEXT HELPERS (Z + opacity)
-- =========================================================
local function ShowText(entity, text)
    if not entity then return end
    ShowAt(entity, Z.text)
    SetTextComponent(entity, "text", text or "")
    SetTextComponent(entity, "opacity", 1.0)
end

local function HideText(entity)
    if not entity then return end
    Hide(entity)
    SetTextComponent(entity, "opacity", 0.0)
    SetTextComponent(entity, "text", "")
end

-- =========================================================
-- NIGHT INIT
-- =========================================================
function HUDSystem:NightInit()
    entityPos = {}

    for i = 1, MAX_SLOTS do
        slotPanels[i]     = FindEntityByName("SlotPanel_"    .. i)
        fishIcons[i]      = FindEntityByName("Icon_Fish_"    .. i)
        orcIcons[i]       = FindEntityByName("Icon_Orc_"     .. i)
        arrivingTexts[i]  = FindEntityByName("ArrivingText_" .. i)
        orderTexts[i]     = FindEntityByName("OrderText_"    .. i)
        patienceFrames[i] = FindEntityByName("PatienceFrame_".. i)
        patienceFills[i]  = FindEntityByName("PatienceFill_" .. i)
        potionSlots[i]    = FindEntityByName("PotionSlot_"   .. i)
        potionIcons[i]    = FindEntityByName("PotionIcon_"   .. i)
        arrowRights[i]    = FindEntityByName("arrowRight_"   .. i)

        CachePos(slotPanels[i])
        CachePos(fishIcons[i])
        CachePos(orcIcons[i])
        CachePos(arrivingTexts[i])
        CachePos(orderTexts[i])
        CachePos(patienceFrames[i])
        CachePos(patienceFills[i])
        CachePos(potionSlots[i])
        CachePos(potionIcons[i])
        CachePos(arrowRights[i])
    end

    barStates = { "green", "green" }

    for i = 1, MAX_SLOTS do
        HUDSystem:HideSlot(i)
    end

    initialized = true
    print("[HUDSystem] NightInit complete.")
end

-- =========================================================
-- ON INIT
-- =========================================================
function HUDSystem:on_init()
    print("[HUDSystem] Script loaded.")
end

-- =========================================================
-- ON UPDATE
-- =========================================================
function HUDSystem:on_update(dt)
    local phase = GetPhase()

    if phase ~= PHASE_NIGHT then
        if initialized then
            for i = 1, MAX_SLOTS do HUDSystem:HideSlot(i) end
            initialized = false
        end
        return
    end

    if not initialized then
        HUDSystem:NightInit()
    end

    local isPaused = HasEngineState("Paused")

    local uiData = GetCustomerUIData()
    if uiData then
        for slot = 1, MAX_SLOTS do
            local data = uiData[slot]
            if data then
                HUDSystem:ShowSlot(slot, data)
            else
                HUDSystem:HideSlot(slot)
            end
        end
    end

    -- Hide text during pause (sprites handled by SceneInterface)
    if isPaused then
        for i = 1, MAX_SLOTS do
            if arrivingTexts[i] then SetTextComponent(arrivingTexts[i], "opacity", 0.0) end
            if orderTexts[i]    then SetTextComponent(orderTexts[i],    "opacity", 0.0) end
        end
    end
end

-- =========================================================
-- SHOW SLOT
-- =========================================================
function HUDSystem:ShowSlot(slot, data)
    ShowAt(slotPanels[slot], Z.panel)
    ShowAt(arrowRights[slot], Z.mid)
    HUDSystem:SetIcon(slot, data.icon_type)

    if data.is_arriving then
        HUDSystem:SetArrivingMode(slot, true)
    else
        HUDSystem:SetArrivingMode(slot, false)
        HUDSystem:SetOrderText(slot, data.order)
        HUDSystem:SetPotionIcon(slot, data.order)
        HUDSystem:SetPatienceBar(slot, data.patience_pct)
    end
end

-- =========================================================
-- HIDE SLOT
-- =========================================================
function HUDSystem:HideSlot(slot)
    Hide(slotPanels[slot])
    Hide(arrowRights[slot])
    Hide(fishIcons[slot])
    Hide(orcIcons[slot])
    Hide(patienceFrames[slot])
    Hide(patienceFills[slot])
    Hide(potionSlots[slot])
    Hide(potionIcons[slot])
    HideText(arrivingTexts[slot])
    HideText(orderTexts[slot])
    barStates[slot] = "green"
end

-- =========================================================
-- SET ICON
-- =========================================================
function HUDSystem:SetIcon(slot, icon_type)
    if icon_type == 1 then
        ShowAt(fishIcons[slot], Z.mid)
        Hide(orcIcons[slot])
    else
        ShowAt(orcIcons[slot], Z.mid)
        Hide(fishIcons[slot])
    end
end

-- =========================================================
-- SET ARRIVING MODE
-- =========================================================
function HUDSystem:SetArrivingMode(slot, is_arriving)
    if is_arriving then
        ShowText(arrivingTexts[slot], "Arriving...")
        HideText(orderTexts[slot])
        Hide(patienceFrames[slot])
        Hide(patienceFills[slot])
        Hide(potionSlots[slot])
        Hide(potionIcons[slot])
    else
        HideText(arrivingTexts[slot])
        ShowAt(patienceFrames[slot], Z.mid)
        ShowAt(potionSlots[slot], Z.mid)
    end
end

-- =========================================================
-- HELPER FUNCTION TO CONVERT THE NAMES OF ORDERS IN THE HUD
-- =========================================================
local function FormatDisplayName(codeName)
    if not codeName then return "" end
    -- 1. Replace underscores with spaces
    local name = codeName:gsub("_", " ")
    -- 2. Capitalize the first letter of each word
    return (name:gsub("(%a)([%w]*)", function(first, rest)
        return first:upper() .. rest:lower()
    end))
end

-- =========================================================
-- SET ORDER TEXT
-- =========================================================
function HUDSystem:SetOrderText(slot, order)
    if not orderTexts[slot] then return end
    
    -- Fetch the friendly display name, matching the logic in Customer.lua
    local displayName = GetItemDisplayName(order) or order
    
    -- Show the formatted name on the HUD panel
    ShowText(orderTexts[slot], displayName)
end

-- =========================================================
-- SET POTION ICON
-- =========================================================
function HUDSystem:SetPotionIcon(slot, order)
    if order and potionIcons[slot] then
        SetTexture(potionIcons[slot], "Assets/Textures/" .. order .. ".png")
        ShowAt(potionIcons[slot], Z.top)
    end
end

-- =========================================================
-- SET PATIENCE BAR
-- =========================================================
function HUDSystem:SetPatienceBar(slot, pct)
    pct = math.max(0.0, math.min(1.0, pct))

    if patienceFills[slot] then
        local currentWidth = pct * BAR_WIDTH_MAX
        local centerX      = BAR_LEFT_EDGE[slot] + (currentWidth / 2)
        SetScale(patienceFills[slot], currentWidth, BAR_HEIGHT, 1.0)
        SetTransformComponent(patienceFills[slot], "position", { x = centerX, y = BAR_CENTER_Y, z = Z.top })

        local targetState
        if pct <= THRESHOLD_RED then
            targetState = "red"
        elseif pct <= THRESHOLD_ORANGE then
            targetState = "orange"
        else
            targetState = "green"
        end

        if barStates[slot] ~= targetState then
            if targetState == "red" then
                SetTexture(patienceFills[slot], TEX_RED)
            elseif targetState == "orange" then
                SetTexture(patienceFills[slot], TEX_ORANGE)
            else
                SetTexture(patienceFills[slot], TEX_GREEN)
            end
            barStates[slot] = targetState
        end
    end
end

return HUDSystem