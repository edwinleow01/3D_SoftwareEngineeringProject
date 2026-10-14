-- DayStatsNextButton.lua
-- Attach to the "Continue" button entity on the daystats screen.
--
-- On click:
--   Days 1-4: calls ApplyUpgradePhase() → proceeds to Upgrade screen
--   Day 5:    calls the Win/Lose check directly (no upgrade screen after final day)

local DayStatsNextButton = {}

local TOTAL_DAYS = 5

function DayStatsNextButton:on_init()
    print("[DayStatsNextButton] on_init, entity:", self.entity)

    RegisterButtonCallback(self.entity, function(ent)
        print("[DayStatsNextButton] Continue clicked.")

        local currentDay = GetCurrentDay()

        if currentDay >= TOTAL_DAYS then
            -- Final day complete — go to Win/Lose
            -- Stats screen has already been shown; now resolve end state
            PopEngineState() -- pop DayStats
            PopEngineState() -- pop Paused

            if WinOrLose() then
                PushEngineState("Win")
            else
                PushEngineState("Lose")
            end
        else
            -- Normal days — proceed to Upgrade screen
            UpdateCurrencyUI() 
            ApplyUpgradePhase()
        end
    end)
end

function DayStatsNextButton:on_update(dt)
    -- Only show/interact while DayStats screen is active
    local visible = HasEngineState("DayStats")
    SetIsUI(self.entity, visible)
end

return DayStatsNextButton
