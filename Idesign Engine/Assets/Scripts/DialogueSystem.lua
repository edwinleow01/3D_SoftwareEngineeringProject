--- day_tutorial_sequenced.lua
--- Sequence: Sheep -> Pear -> Herb -> Gate
--- Uses Engine-side persistence to prevent re-plays

local DayTutorial = {}
SetData("dayTutorialPlayed", 0)
SetData("mapSelectionTutorialPlayed", 0)
SetData("quarryUnlockAnnouncePlayed", 0)

-- Add these to your DayTutorial table at the top
DayTutorial.originalYaw = 0
DayTutorial.originalPitch = 0

DayTutorial.waitingForLanding    = false
DayTutorial.waitingForDialogue   = false
DayTutorial.sequenceStep         = 1
DayTutorial.skipKeyHeld          = false

DayTutorial.mapSelWaitingForDialogue     = false
DayTutorial.quarryUnlockWaitingForDialogue = false

-- Snapshot timers: record the RealTime when each phase first becomes active
DayTutorial.dayStartTime            = nil
DayTutorial.mapSelStartTime         = nil
DayTutorial.quarryUnlockStartTime   = nil

DayTutorial.hasSavedInitialRot = false

local DAY_DELAY    = 1.65 -- custo mto fit loading and timing
local MAPSEL_DELAY = 0.5

function DayTutorial:on_init()
    print("[DayTutorial] Sequenced Script Initialized. Checking persistent flags...")
end

function DayTutorial:SkipSequence()
    print("[DayTutorial] Sequence skipped via F9.")
    SetData("dayTutorialPlayed", 1)
    if self.infoBG then DestroyEntity(self.infoBG) end
    if self.infoUI then DestroyEntity(self.infoUI) end
    if CutsceneIsFlying() then CutsceneStop() end
    if DialogueIsActive() then DialogueClear() end
    self.waitingForLanding  = false
    self.waitingForDialogue = false
    self.sequenceStep       = 5
    self.dayStartTime       = nil
    print("[DayTutorial] Full sequence complete. Player control restored.")
end

function DayTutorial:SkipMapSelSequence()
    print("[DayTutorial] MapSelection sequence skipped via F9.")
    SetData("mapSelectionTutorialPlayed", 1)
    if DialogueIsActive() then DialogueClear() end
    self.mapSelWaitingForDialogue = false
    self.mapSelStartTime          = nil
    print("[DayTutorial] MapSelection sequence complete.")
end

function DayTutorial:SkipQuarryUnlockSequence()
    print("[DayTutorial] Quarry unlock announcement skipped via F9.")
    SetData("quarryUnlockAnnouncePlayed", 1)
    if DialogueIsActive() then DialogueClear() end
    self.quarryUnlockWaitingForDialogue = false
    self.quarryUnlockStartTime          = nil
    print("[DayTutorial] Quarry unlock announcement complete.")
end

function DayTutorial:on_update(dt)
    local inGameplay     = IsEngineState("Play")
    local inMapSelection = IsEngineState("MapSelection")
    local isDayPhase     = (GetPhase() == 1)
    local playedFlag     = GetData("dayTutorialPlayed")
    local mapSelFlag          = GetData("mapSelectionTutorialPlayed")
    local quarryAnnounceFlag  = GetData("quarryUnlockAnnouncePlayed")
    local now                 = GetRealTime()

    -- -------------------------------------------------------
    -- Cleanup for le recipe book tutorial
    -- -------------------------------------------------------
    if self.waitingForDialogue and not DialogueIsActive() then
        self.waitingForDialogue = false
        
        -- CLEANUP: If we just finished step 5, remove the prefabs
        if self.sequenceStep == 5 then
            if self.infoBG then DestroyEntity(self.infoBG) end
            if self.infoUI then DestroyEntity(self.infoUI) end
            print("[DayTutorial] UI Prefabs cleared.")
        end

        self.sequenceStep = self.sequenceStep + 1
        if self.sequenceStep <= 5 then -- Updated limit to 5
            self:PlayNextStep()
        else
            print("[DayTutorial] Full sequence complete. Player control restored.")
            print("[DayTutorial] Restoring rotation...")
        
            SetCameraRot(0, 0) 
            -- 3. (Optional) Re-sync the Player Physics body 
            -- If the body is still facing the Gate, it might pull the camera back
            local px, py, pz = GetPlayerPos() -- If you have this export
            TeleportPlayer(px, py, pz, 0, 0, 0)
        end
    end
    


    -- F9 skip with debounce
    local skipPressed = IsKeyDown(298)
    if skipPressed and not self.skipKeyHeld then
        if inGameplay and isDayPhase and (playedFlag == 0 or self.waitingForLanding or self.waitingForDialogue) then
            self.skipKeyHeld = true
            self:SkipSequence()
            return
        end
        if inMapSelection and (mapSelFlag == 0 or self.mapSelWaitingForDialogue) then
            self.skipKeyHeld = true
            self:SkipMapSelSequence()
            return
        end
        if inMapSelection and (quarryAnnounceFlag == 0 or self.quarryUnlockWaitingForDialogue) then
            self.skipKeyHeld = true
            self:SkipQuarryUnlockSequence()
            return
        end
    end
    if not skipPressed then self.skipKeyHeld = false end

    -- -------------------------------------------------------
    -- Day tutorial: snapshot start time on first eligible frame
    -- -------------------------------------------------------
    if inGameplay and isDayPhase and playedFlag == 0 then
        if not self.dayStartTime then
            self.dayStartTime = now
            
        end
        if now - self.dayStartTime >= DAY_DELAY then
            if not self.hasSavedInitialRot then
                self.origPitch, self.origYaw = GetCameraRot() 
                self.hasSavedInitialRot = true -- Lock it!
                print("[DayTutorial] PERMANENT Starting Rotation Saved: " .. self.origYaw)
            end

            self.dayStartTime = nil
            SetData("dayTutorialPlayed", 1)
            print("[DayTutorial] First time day detected. Starting sequence.")
            self:PlayNextStep()
        end
    else
        self.dayStartTime = nil  -- reset if we leave the condition
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
            print("[DayTutorial] Full sequence complete. Player control restored.")

        end
    end


    -- -------------------------------------------------------
    -- MapSelection tutorial: snapshot start time on first eligible frame
    -- Uses GetRealTime so dt=0 from engine is irrelevant
    -- -------------------------------------------------------
    if inMapSelection and mapSelFlag == 0 then
        if not self.mapSelStartTime then
            self.mapSelStartTime = now
        end
        if now - self.mapSelStartTime >= MAPSEL_DELAY then
            self.mapSelStartTime = nil
            SetData("mapSelectionTutorialPlayed", 1)
            print("[DayTutorial] First time MapSelection detected. Starting dialogue.")
            self:PlayMapSelDialogue()
        end
    else
        self.mapSelStartTime = nil  -- reset if we leave MapSelection
    end

    if self.mapSelWaitingForDialogue and not DialogueIsActive() then
        self.mapSelWaitingForDialogue = false
        print("[DayTutorial] MapSelection dialogue complete.")
    end

    -- -------------------------------------------------------
    -- Quarry unlock announcement: fires once on MapSelection when quarry is available
    -- Waits for the same MAPSEL_DELAY to avoid fighting loading
    -- -------------------------------------------------------
    if inMapSelection and quarryAnnounceFlag == 0 and IsQuarryUnlocked() then
        if not self.quarryUnlockStartTime then
            self.quarryUnlockStartTime = now
        end
        if now - self.quarryUnlockStartTime >= MAPSEL_DELAY then
            self.quarryUnlockStartTime = nil
            SetData("quarryUnlockAnnouncePlayed", 1)
            print("[DayTutorial] Quarry unlocked and MapSelection open. Playing announcement.")
            self:PlayQuarryUnlockDialogue()
        end
    else
        if not (inMapSelection and quarryAnnounceFlag == 0) then
            self.quarryUnlockStartTime = nil  -- reset if we leave the condition
        end
    end

    if self.quarryUnlockWaitingForDialogue and not DialogueIsActive() then
        self.quarryUnlockWaitingForDialogue = false
        print("[DayTutorial] Quarry unlock announcement complete.")
    end
end

function DayTutorial:PlayNextStep()
    local target = ""
    local offX, offY, offZ = 0, 5, 8

    if self.sequenceStep == 1 then
        target = "Sheep"
    elseif self.sequenceStep == 2 then
        target = "Pear"
        offX, offY, offZ = 0, 3, 5
    elseif self.sequenceStep == 3 then
        target = "Herb"
    elseif self.sequenceStep == 4 then
        target = "Gate"
        offX, offY, offZ = 25, 6, 12
    elseif self.sequenceStep == 5 then -- Fixed: used elseif (one word)
        print("[DayTutorial] Stage 5 | Spawning Recipe Book Info Prefabs")
        self.infoBG = SpawnPrefab("RecipeBook_Info_BG_Prefab")
        self.infoUI = SpawnPrefab("RecipeBook_Info_Prefab")
        -- Since there is no cutscene for step 5, trigger dialogue manually
        self:StartDialogueForCurrentStep() 
        return -- Exit early so we don't run the Cutscene code below
    end

    -- This part only runs for steps 1-4
    print("[DayTutorial] Stage " .. self.sequenceStep .. " | Flying to: " .. target)
    CutsceneSetTarget(target)
    CutsceneSetOffset(offX, offY, offZ)
    CutsceneSetSpeed(2.5)
    CutsceneStart()
    self.waitingForLanding = true
end

function DayTutorial:StartDialogueForCurrentStep()
    DialogueClear()

    if self.sequenceStep == 1 then
        DialogueAdd("Look at these fluffy sheep! They aren't just for show.", "Aunt Mira", "pear", "event:/DialogueForest1")
        DialogueAdd("Gather their wool, it may prove useful in certain recipes!", "Aunt Mira", "portrait", "event:/DialogueForest2")
    elseif self.sequenceStep == 2 then
        DialogueAdd("Ah, a fine pear! Some recipes require more than just roots.", "Aunt Mira", "pear", "event:/DialogueForest3")
        DialogueAdd("Keep your eyes peeled for fruit hanging in the brush.", "Aunt Mira", "portrait", "event:/DialogueForest4")
    elseif self.sequenceStep == 3 then
        DialogueAdd("These are the herbs you need for your remedies.", "Aunt Mira", "pear", "event:/DialogueForest5")
        DialogueAdd("Gather them by pressing 'E' until your bag is heavy.", "Aunt Mira", "portrait", "event:/DialogueForest6")
    elseif self.sequenceStep == 4 then
        DialogueAdd("Finally, the Gate. Use it once you are finished here.", "Aunt Mira", "pear", "event:/DialogueForest7")
    elseif self.sequenceStep == 5 then
        DialogueAdd("Before you go, take a look at your Recipe Book.", "Aunt Mira", "portrait", "event:/DialogueForest8")
        DialogueAdd("It shows you what ingredients you've found and, more importantly, what they are worth as potions!", "Aunt Mira", "portrait", "event:/DialogueForest9")
        DialogueAdd("You can open it anytime by pressing 'R'. Now, get to work!", "Aunt Mira", "portrait", "event:/DialogueForest10")
    end

    DialogueStart()
    self.waitingForDialogue = true
end

function DayTutorial:PlayMapSelDialogue()
    DialogueClear()
    DialogueAdd("This is the map. Every location you unlock will show itself here.", "Aunt Mira", "portrait", "event:/DialogueMap1")
    DialogueAdd("Start with the forest — herbs won't gather themselves, and we've potions to brew.", "Aunt Mira", "portrait", "event:/DialogueMap2")
    DialogueStart()
    self.mapSelWaitingForDialogue = true
end

function DayTutorial:PlayQuarryUnlockDialogue()
    DialogueClear()
    DialogueAdd("The Quarry is open to you now, child. Rich in ore and rare materials.", "Aunt Mira", "portrait", "event:/DialogueMap3")
    DialogueAdd("Head there when you need minerals. Some of our finer recipes demand nothing less.", "Aunt Mira", "portrait", "event:/DialogueMap4")
    DialogueStart()
    self.quarryUnlockWaitingForDialogue = true
end

return DayTutorial