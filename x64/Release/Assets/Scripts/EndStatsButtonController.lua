-- EndStatsButtons.lua
-- Attach to a controller entity near the endstats screen.
-- Manages the two buttons visible on the Journey's End screen.
--
-- Entity names (match your scene prefabs):
--   "EndStats_MainMenuBtn"
--   "EndStats_ViewCreditsBtn"
-- Both must have ButtonComponent attached.

local EndStatsButtons = {}

local BTN_MAIN_MENU    = "EndStats_MainMenuBtn"
local BTN_VIEW_CREDITS = "EndStats_ViewCreditsBtn"

local mainMenuBtn    = nil
local viewCreditsBtn = nil

function EndStatsButtons:on_init()
    print("[EndStatsButtons] on_init, entity:", self.entity)

    mainMenuBtn    = FindEntityByName(BTN_MAIN_MENU)
    viewCreditsBtn = FindEntityByName(BTN_VIEW_CREDITS)

    -- Populate the end screen text the moment this script initialises
    -- (the EndStats state was already pushed by SceneInterface before this runs).
    -- DayStatsScreen.PopulateEndScreen()

    if mainMenuBtn then
        RegisterButtonCallback(mainMenuBtn, function(ent)
            print("[EndStatsButtons] Main Menu clicked.")
            -- Clear everything and return to Menu
            ClearEngineStates()
            PushEngineState("Menu")
        end)
    else
        print("[EndStatsButtons] WARNING: " .. BTN_MAIN_MENU .. " not found.")
    end

    if viewCreditsBtn then
        RegisterButtonCallback(viewCreditsBtn, function(ent)
            print("[EndStatsButtons] View Credits clicked.")
            -- Pop EndStats, push Credit
            PopEngineState()
            PushEngineState("Credit")
        end)
    else
        print("[EndStatsButtons] WARNING: " .. BTN_VIEW_CREDITS .. " not found.")
    end
end

function EndStatsButtons:on_update(dt)
    local visible = HasEngineState("EndStats")
    if mainMenuBtn    then SetIsUI(mainMenuBtn,    visible) end
    if viewCreditsBtn then SetIsUI(viewCreditsBtn, visible) end
end

return EndStatsButtons
