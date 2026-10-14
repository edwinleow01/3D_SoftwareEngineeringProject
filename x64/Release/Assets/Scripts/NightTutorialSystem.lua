--- night_tutorial_sequenced.lua
--- Uses Engine-side persistence to prevent re-plays on soft resets
--- Sequence: Frog -> Cutting_Board -> Mortar -> Cauldron -> Furnace

local NightTutorial = {}
SetData("nightTutorialPlayed", 0)

NightTutorial.waitingForLanding = false
NightTutorial.waitingForDialogue = false
NightTutorial.timer = 0.0
NightTutorial.delayDuration = 1.35 
NightTutorial.sequenceStep = 1
NightTutorial.skipKeyHeld = false

SetData("nightTutorialPlayed", 0)

function NightTutorial:on_init()
    print("[NightTutorial] Night sequence initialized. Checking persistent flags...")
end

function NightTutorial:SkipSequence()
    print("[NightTutorial] Sequence skipped via F9.")

    SetData("nightTutorialPlayed", 1)

    if CutsceneIsFlying() then
        CutsceneStop()
    end
    if DialogueIsActive() then
        DialogueClear()
    end

    self.waitingForLanding = false
    self.waitingForDialogue = false
    self.sequenceStep = 6  -- Push past the last step (5)
    self.timer = 0.0

    print("[NightTutorial] Night sequence complete. Control restored.")
end

function NightTutorial:on_update(dt)
    local inGameplay = IsEngineState("Play")
    local isNightPhase = (GetPhase() == 2)
    local playedFlag = GetData("nightTutorialPlayed")

    -- F9 skip with debounce: only fires once per key press
    local skipPressed = IsKeyDown(298)
    if skipPressed and not self.skipKeyHeld and inGameplay and isNightPhase then
        if playedFlag == 0 or self.waitingForLanding or self.waitingForDialogue then
            self.skipKeyHeld = true
            self:SkipSequence()
            return
        end
    end
    if not skipPressed then
        self.skipKeyHeld = false
    end

    if inGameplay and isNightPhase and playedFlag == 0 then
        self.timer = self.timer + dt
        if self.timer >= self.delayDuration then
            SetData("nightTutorialPlayed", 1)
            print("[NightTutorial] First time night detected. Starting sequence.")
            self:PlayNextStep()
        end
    end

    if self.waitingForLanding and not CutsceneIsFlying() then
        self.waitingForLanding = false
        self:StartDialogueForCurrentStep()
    end

    if self.waitingForDialogue and not DialogueIsActive() then
        self.waitingForDialogue = false
        self.sequenceStep = self.sequenceStep + 1
        
        if self.sequenceStep <= 5 then
            self:PlayNextStep()
        else
            print("[NightTutorial] Night sequence complete. Control restored.")
            SetCameraRot(0, 0) 
            local px, py, pz = GetPlayerPos() -- If you have this export
            TeleportPlayer(px, py, pz, 0, 90, 0)
        end
    end
end

function NightTutorial:PlayNextStep()
    local target = ""
    local offX, offY, offZ = 0, 0, 0
    
    if self.sequenceStep == 1 then
        target = "Frog"
        offX, offY, offZ = 3.979, 1.59, 0.429
    elseif self.sequenceStep == 2 then
        target = "Cutting_Board"
        offX, offY, offZ = 2.0, 4.0, 4.0
    elseif self.sequenceStep == 3 then
        target = "Mortar"
        offX, offY, offZ = 2.0, 4.0, 4.0
    elseif self.sequenceStep == 4 then
        target = "Furnace"
        offX, offY, offZ = 6.2, 2.1, 0.1
    elseif self.sequenceStep == 5 then
        target = "Cauldron"          -- matches the scene entity name exactly
        offX, offY, offZ = -2.0, 4.5, -4.0

    end

    print("[NightTutorial] Flying to: " .. target)
    CutsceneSetTarget(target)
    CutsceneSetOffset(offX, offY, offZ)
    CutsceneSetSpeed(2.5)
    CutsceneStart()

    self.waitingForLanding = true
end

function NightTutorial:StartDialogueForCurrentStep()
    DialogueClear()
    if self.sequenceStep == 1 then
        DialogueAdd("Look alive, child! The moon is high; 'tis the Night Phase now.", "Aunt Mira", "pear", "event:/DialogueShop1")
        DialogueAdd("Don't just stand, customers awaits!", "Aunt Mira", "portrait", "event:/DialogueShop2")
    elseif self.sequenceStep == 2 then
        DialogueAdd("First, preparation! Use the knife!", "Aunt Mira", "pear", "event:/DialogueShop3")
        DialogueAdd("Hold the knife and click the mouse in time to dice em up!", "Aunt Mira", "pear", "event:/DialogueShop4")
    elseif self.sequenceStep == 3 then
        DialogueAdd("The Mortar is for crushing ingredients into powder.", "Aunt Mira", "pear", "event:/DialogueShop5")
        DialogueAdd("Click the circles as they appear to grind your ingredients down!", "Aunt Mira", "portrait", "event:/DialogueShop6")
    elseif self.sequenceStep == 4 then
         DialogueAdd("The Furnace melts your ores into refined bars.", "Aunt Mira", "pear", "event:/DialogueShop7")
        DialogueAdd("Some recipes demand metal. Feed it well!", "Aunt Mira", "portrait", "event:/DialogueShop8")
    elseif self.sequenceStep == 5 then
        DialogueAdd("Then, the cauldron. Toss your ingredients in and stir!", "Aunt Mira", "pear", "event:/DialogueShop9")
        DialogueAdd("Press 'R' if you've forgotten your potions and need the Recipe Book.", "Aunt Mira", "portrait", "event:/DialogueShop10")

    end
    DialogueStart()
    self.waitingForDialogue = true
end

return NightTutorial