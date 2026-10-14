
local M = {}

function M:on_init()
    RegisterButtonCallback(self.entity, function(ent)
        local didWin = WinOrLose()
        if didWin then
            PushEngineState("Win")
        else
            PushEngineState("Lose")
        end
    end)
end
return M