-- Customer.lua
local Customer = {}

Customer.interaction_range = 2.5

-- Textbox configuration
local prefab_textbox   = "CustomerTextbox_Prefab"
local textbox_y_offset = 4.5

-- Log prefab
local prefab_log = "CustomerLog_Prefab"

-- Interaction cooldown
local interaction_cooldown = 0.7

-- Wrong item tracking
local MAX_WRONG_ATTEMPTS = 2

-- =========================================================
-- ANIMATION CONSTANTS & DURATIONS
-- =========================================================
local ANIM_SERVE_CORRECT = "Armature|Armature|ServeCorrect"
local ANIM_SERVE_WRONG   = "Armature|Armature|ServeWrong"
local ANIM_TIMEOUT       = "Armature|Armature|TimeOut"

-- Map the exact millisecond duration by Customer Type!
-- Type 1 = Fish, Type 2 = Goblin/Orc
-- Update these numbers based on your exact Blender frame counts (Frames x 40)
local ANIM_DURATIONS = {
    [1] = { -- THE FISH
        [ANIM_SERVE_CORRECT] = 2000, -- e.g., 25 frames
        [ANIM_SERVE_WRONG]   = 2400, -- e.g., 40 frames
        [ANIM_TIMEOUT]       = 3400  -- e.g., 80 frames
    },
    [2] = { -- THE GOBLIN
        [ANIM_SERVE_CORRECT] = 1400, -- e.g., 35 frames
        [ANIM_SERVE_WRONG]   = 2800, -- e.g., 70 frames
        [ANIM_TIMEOUT]       = 2000  -- e.g., 50 frames
    }
}

-- =========================================================
-- HELPERS: per-entity state via SetData/GetData
-- =========================================================
local function GetState(entity, key)
    return GetData(key .. "_" .. tostring(entity))
end

local function SetState(entity, key, value)
    SetData(key .. "_" .. tostring(entity), value)
end

local function ClearState(entity)
    local id = tostring(entity)
    SetData("HasReceived_"      .. id, 0)
    SetData("HasArrived_"       .. id, 0)
    SetData("WrongCount_"       .. id, 0)
    SetData("LastInteractTime_" .. id, 0)
    SetData("ElapsedTime_"      .. id, 0)
    SetData("PendingDestroy_"   .. id, 0) -- Added for animation delay
    -- CustomerOrder string is managed by CustomerSystem (not SetData)
    SetData("Textbox_"          .. id, 0)
end

-- =========================================================
-- ON INIT
-- =========================================================
function Customer.on_init(self)
    print("[Customer] ===== INIT DEBUG =====")
    print("[Customer] Initialized for entity: " .. tostring(self.entity))

    -- Clear any leftover state
    ClearState(self.entity)

    local order = GetCustomerOrder(self.entity)
    if not order or order == "" then
        print("[Customer] WARNING: No order from CustomerSystem! Using fallback.")
        order = "strength_potion"
    end

    print("[Customer] Order confirmed: " .. tostring(order))
    print("[Customer] State initialized for entity: " .. tostring(self.entity))
end

-- =========================================================
-- ON UPDATE
-- =========================================================
function Customer.on_update(self, dt)
    -- Tick elapsed time in milliseconds
    local elapsed_ms = GetState(self.entity, "ElapsedTime") + math.floor(dt * 1000)
    SetState(self.entity, "ElapsedTime", elapsed_ms)

    -- Handle delayed destruction so animations have time to play!
    local pending_destroy_time = GetState(self.entity, "PendingDestroy")
    if pending_destroy_time > 0 and elapsed_ms >= pending_destroy_time then
        DestroyEntity(self.entity)
        SetState(self.entity, "PendingDestroy", 0)
    end
end

-- =========================================================
-- ON ARRIVAL
-- =========================================================
function Customer.OnArrival(senderID, targetID, actionType)
    print("[Customer] Entity " .. tostring(targetID) .. " arrived at counter!")

    SetState(targetID, "HasArrived", 1)

    local orderIDString = GetCustomerOrder(targetID) or "strength_potion"
    local displayName = GetItemDisplayName(orderIDString)
    local orderText = "Hi, I want a " .. tostring(displayName) .. "."
    
    Customer.SpawnTextbox(targetID, orderText)
    Customer.SpawnLog("Customer arrived!")
end

-- =========================================================
-- ON INTERACT
-- =========================================================
function Customer.OnInteract(senderID, targetID, actionType)
    local customer_entity = targetID

    -- Check arrived
    if GetState(customer_entity, "HasArrived") ~= 1 then
        print("[Customer] Not ready yet! Still walking to the counter...")
        return
    end

    -- Check cooldown
    local elapsed_ms      = GetState(customer_entity, "ElapsedTime")
    local lastInteract_ms = GetState(customer_entity, "LastInteractTime")
    local time_since_last = (elapsed_ms - lastInteract_ms) / 1000.0

    if time_since_last < interaction_cooldown then
        print("[Customer] Interaction on cooldown")
        return
    end

    SetState(customer_entity, "LastInteractTime", elapsed_ms)
    
    -- Already served
    if GetState(customer_entity, "HasReceived") == 1 then
        print("[Customer] Customer already has their order!")
        return
    end

    -- Check held item
    local heldItemId = GetSelectedItem()
    if heldItemId < 0 then
        print("[Customer] You're not holding anything!")
        Customer.SpawnLog("You're not holding anything!")
        return
    end

    local itemName = GetItemNameById(heldItemId)
    local my_order = GetCustomerOrder(customer_entity)
    local myType   = GetData("Type_" .. tostring(customer_entity))
    
    -- Fallback safety if type is missing
    if not myType or myType < 1 then myType = 1 end

    -- =====================
    -- CORRECT ITEM
    -- =====================
    if itemName == my_order then
        print("[Customer] Perfect! That's exactly what I wanted!")
        SetState(customer_entity, "HasReceived", 1)

        local reward = GetItemValue(heldItemId) 
        if not reward or reward <= 0 then reward = 100 end

        ConsumeSelectedItem()
        Customer.DestroyTextbox(customer_entity)

        AddCurrency(reward)
        AddCustomerServed()   -- [STATS]
        UpdateCurrencyUI()
        Customer.SpawnLog("Customer served! +" .. reward .. " gold")

        FireEvent("CustomerComplete", customer_entity, senderID, 1)

        if myType == 1 then
            Sound.Play("event:/FishCorrect")
            Sound.Play("event:/EarnGold")
        else
            Sound.Play("event:/OrcCorrect2")
            Sound.Play("event:/EarnGold")
        end
        
        -- TURN OFF LOOPING, PLAY ANIMATION, AND DELAY DESTRUCTION
        SetAnimationSettings(customer_entity, 1.0, false, false)
        PlayEntityAnimation(customer_entity, ANIM_SERVE_CORRECT)
        
        -- Look up exact time based on Customer Type
        local exact_time = ANIM_DURATIONS[myType][ANIM_SERVE_CORRECT] or 2000
        SetState(customer_entity, "PendingDestroy", elapsed_ms + exact_time)

    -- =====================
    -- WRONG ITEM
    -- =====================
    else
        local wrong_count = GetState(customer_entity, "WrongCount") + 1
        SetState(customer_entity, "WrongCount", wrong_count)

        if myType == 1 then
            Sound.Play("event:/FishWrong")
        else
            Sound.Play("event:/OrcWrong2")
        end
        
        local niceName = GetItemDisplayName(my_order)
        
        if wrong_count >= MAX_WRONG_ATTEMPTS then
            print("[Customer] That's it! I've had enough! Leaving...")
            Customer.DestroyTextbox(customer_entity)
            
            local penalty = 100
            AddCurrency(-penalty)
            AddCustomerFailed()
            UpdateCurrencyUI()
            Customer.SpawnLog("Customer left angry! -" .. penalty .. " gold")
            FireEvent("CustomerFailed", customer_entity, senderID, 0)
            
            -- TURN OFF LOOPING, PLAY ANIMATION, AND DELAY DESTRUCTION
            SetAnimationSettings(customer_entity, 1.0, false, false)
            PlayEntityAnimation(customer_entity, ANIM_SERVE_WRONG)
            
            -- Look up exact time based on Customer Type
            local exact_time = ANIM_DURATIONS[myType][ANIM_SERVE_WRONG] or 2000
            SetState(customer_entity, "PendingDestroy", elapsed_ms + exact_time)
        else
            local chances_left = MAX_WRONG_ATTEMPTS - wrong_count
            print("[Customer] Sorry, I ordered " .. tostring(niceName) .. ", not " .. tostring(itemName) .. "!")
            Customer.SpawnLog("Wrong item! " .. chances_left .. " chance(s) left")
            -- Stays in idle for first warning
        end
    end
end

-- =========================================================
-- ON TIMEOUT
-- =========================================================
function Customer.OnTimeout(senderID, targetID, actionType)
    print("[Customer] Entity " .. tostring(targetID) .. " timed out! Cleaning up...")
    Customer.DestroyTextbox(targetID)
    
    local myType = GetData("Type_" .. tostring(targetID))
    if not myType or myType < 1 then myType = 1 end
    
    -- TURN OFF LOOPING AND PLAY TIMEOUT ANIMATION
    SetAnimationSettings(targetID, 1.0, false, false)
    PlayEntityAnimation(targetID, ANIM_TIMEOUT)
    
    -- Delay destruction for the timeout animation as well!
    local elapsed_ms = GetState(targetID, "ElapsedTime")
    local exact_time = ANIM_DURATIONS[myType][ANIM_TIMEOUT] or 2000
    SetState(targetID, "PendingDestroy", elapsed_ms + exact_time)
end

-- =========================================================
-- ORDER GENERATION
-- =========================================================
function Customer.GenerateRequest()
    local fixedQueue = GetData("fixedQueue")

    if fixedQueue > 0 then
        local items = {"warmth_potion", "vitality_potion", "strength_potion", "mending_potion", "defense_potion", "preservation_powder", "healing_powder"}
        local item  = items[fixedQueue] or "strength_potion"
        SetData("fixedQueue", fixedQueue - 1)
        return item
    else
        local requestedItem = GetRandomCraftablePotion()
        if not requestedItem then
            requestedItem = "strength_potion"
        end
        return requestedItem
    end
end

-- =========================================================
-- TEXTBOX FUNCTIONS
-- =========================================================
function Customer.SpawnTextbox(entity, text)
    local textboxKey = "Textbox_" .. tostring(entity)
    local existing = GetData(textboxKey)
    if existing and existing > 0 then return end

    local textbox = SpawnPrefab(prefab_textbox, entity)
    if not textbox then return end

    SetTextComponent(textbox, "text", text)
    SetData(textboxKey, textbox)

    local cx, cy, cz = GetPosition(entity)
    SetPosition(textbox, cx, cy + textbox_y_offset, cz)
end

function Customer.DestroyTextbox(entity)
    local textboxKey = "Textbox_" .. tostring(entity)
    local textbox    = GetData(textboxKey)
    if textbox and textbox > 0 then
        DestroyEntity(textbox)
        SetData(textboxKey, 0)
    end
end

-- =========================================================
-- LOG
-- =========================================================
function Customer.SpawnLog(message)
    local logEntity = SpawnPrefab(prefab_log)
    if logEntity then
        SetTextComponent(logEntity, "text", message)
    end
end

return Customer