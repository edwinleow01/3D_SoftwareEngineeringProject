local M = {}

function M:on_init()
    print("[NextDayButton] on_init called for entity:", self.entity)

    RegisterButtonCallback(self.entity, function(ent)
        print("[NextDayButton] Next Day clicked!")

        -- tell map UI to reset icon on next open
        SetData("ResetMapIconNextOpen", 1)

        -- Leave Upgrade
        PopEngineState()
        PushEngineState("Play")

        -- Advance day + push proper day transition
        AdvanceDayAndPushTransition()

        print("[NextDayButton] Day advanced and transition pushed")
    end)
end

return M