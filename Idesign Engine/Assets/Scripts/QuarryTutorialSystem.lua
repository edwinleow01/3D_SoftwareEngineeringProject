--- QuarryTutorialSystem.lua
--- Plays a camera onboarding sequence the first time the player enters the Quarry.
--- Triggered by MapSelectionUI.lua flicking "quarryFirstEntered" on travel confirm.
--- Pattern mirrors NightTutorialSystem.lua / DialogueSystem.lua.

local QuarryTutorial = {}

-- Initialise persistent flags (engine resets these on fresh run)
SetData("quarryFirstEntered",  0)
SetData("quarryTutorialPlayed", 0)

QuarryTutorial.waitingForLanding  = false
QuarryTutorial.waitingForDialogue = false
QuarryTutorial.timer              = 0.0
QuarryTutorial.delayDuration      = 1.5   -- seconds after arrival before sequence starts
QuarryTutorial.sequenceStep       = 1
QuarryTutorial.skipKeyHeld        = false

-- =========================================================
-- Total number of tutorial steps (update if you add/remove steps)
-- =========================================================
local TOTAL_STEPS = 3

-- =========================================================
-- Step 1 flies to copper (welcome + copper dialogue).
-- Step 2 flies to iron.
-- =========================================================
local STEP_TARGETS = {
    [1] = { entity = "copper", offX =  2.0, offY = 4.0, offZ =  4.0 },     -- copper deposit
    [2] = { entity = "iron",   offX = -2.0, offY = 3.5, offZ =  4.0 },     -- iron deposit
    [3] = { entity = "minesign",   offX = -10.0, offY = 10.5, offZ = 10.0 }, 
}

-- =========================================================
function QuarryTutorial:on_init()
    print("[QuarryTutorial] Initialized.")
end

-- =========================================================
function QuarryTutorial:SkipSequence()
    print("[QuarryTutorial] Sequence skipped via F9.")

    SetData("quarryTutorialPlayed", 1)

    if CutsceneIsFlying() then CutsceneStop() end
    if DialogueIsActive() then DialogueClear() end

    self.waitingForLanding  = false
    self.waitingForDialogue = false
    self.sequenceStep       = TOTAL_STEPS + 1
    self.timer              = 0.0

    print("[QuarryTutorial] Sequence complete. Control restored.")
end

-- =========================================================
function QuarryTutorial:on_update(dt)
    local inGameplay    = IsEngineState("Play")
    local isDayPhase    = (GetPhase() == 1)
    local enteredFlag   = GetData("quarryFirstEntered")
    local playedFlag    = GetData("quarryTutorialPlayed")

    -- F9 skip with debounce
    local skipPressed = IsKeyDown(298)
    if skipPressed and not self.skipKeyHeld and inGameplay and isDayPhase then
        if enteredFlag == 1 and (playedFlag == 0 or self.waitingForLanding or self.waitingForDialogue) then
            self.skipKeyHeld = true
            self:SkipSequence()
            return
        end
    end
    if not skipPressed then self.skipKeyHeld = false end

    -- Wait for first eligible frame after arriving in quarry, then start sequence
    if inGameplay and isDayPhase and enteredFlag == 1 and playedFlag == 0 then
        self.timer = self.timer + dt
        if self.timer >= self.delayDuration then
            SetData("quarryTutorialPlayed", 1)
            print("[QuarryTutorial] First quarry entry detected. Starting sequence.")
            self:PlayNextStep()
        end
    end

    -- Advance after camera lands
    if self.waitingForLanding and not CutsceneIsFlying() then
        self.waitingForLanding = false
        self:StartDialogueForCurrentStep()
    end

    -- Advance after dialogue finishes
    if self.waitingForDialogue and not DialogueIsActive() then
        self.waitingForDialogue = false
        self.sequenceStep = self.sequenceStep + 1
        if self.sequenceStep <= TOTAL_STEPS then
            self:PlayNextStep()
        else
            print("[QuarryTutorial] Sequence complete. Control restored.")
            SetCameraRot(0, 0) 
            -- 3. (Optional) Re-sync the Player Physics body 
            -- If the body is still facing the Gate, it might pull the camera back
            local px, py, pz = GetPlayerPos() -- If you have this export
            TeleportPlayer(px, py, pz, 0, 90, 0)
        end
    end
end

-- =========================================================
function QuarryTutorial:PlayNextStep()
    local step = STEP_TARGETS[self.sequenceStep]
    if not step then
        print("[QuarryTutorial] WARNING: no target defined for step " .. self.sequenceStep)
        return
    end

    print("[QuarryTutorial] Step " .. self.sequenceStep .. " | Flying to: " .. step.entity)
    CutsceneSetTarget(step.entity)
    CutsceneSetOffset(step.offX, step.offY, step.offZ)
    CutsceneSetSpeed(1.8)
    CutsceneStart()
    self.waitingForLanding = true
end

-- =========================================================
function QuarryTutorial:StartDialogueForCurrentStep()
    DialogueClear()

    if self.sequenceStep == 1 then
        DialogueAdd("Welcome to the Quarry. A fine place to stock up on rare minerals for more elaborate potions!", "Aunt Mira", "portrait", "event:/DialogueQuarry1")
        DialogueAdd("Copper! A sturdy ore, useful in many of our recipes.", "Aunt Mira", "pear", "event:/DialogueQuarry2")
        DialogueAdd("Press 'E' near any ore to mine it. Don't leave empty-handed!", "Aunt Mira", "portrait", "event:/DialogueQuarry3")
    elseif self.sequenceStep == 2 then
        DialogueAdd("And here's iron — tricky to refine but powerful in the right potion.", "Aunt Mira", "pear", "event:/DialogueQuarry4")
        DialogueAdd("Fill that bag and head back when you're done.", "Aunt Mira", "portrait", "event:/DialogueQuarry5")
    elseif self.sequenceStep == 3 then
        DialogueAdd("This is the exit for the mine", "Aunt Mira", "pear", "event:/DialogueQuarry6")
        DialogueAdd("Make sure you don't leave anything behind!", "Aunt Mira", "portrait", "event:/DialogueQuarry7")
    end

    DialogueStart()
    self.waitingForDialogue = true
end

return QuarryTutorial
