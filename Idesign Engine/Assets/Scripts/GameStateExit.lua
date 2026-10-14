
local M = {}

function M:on_init()
    RegisterButtonCallback(self.entity, function(ent)
        GameStateExit()
    end)
end

return M