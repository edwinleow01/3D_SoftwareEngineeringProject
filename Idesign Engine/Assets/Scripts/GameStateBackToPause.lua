
local M = {}

function M:on_init()
    RegisterButtonCallback(self.entity, function(ent)
        GameStateBackToPause()
    end)
end

return M