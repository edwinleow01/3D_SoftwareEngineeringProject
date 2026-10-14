
local M = {}

function M:on_init()
    RegisterButtonCallback(self.entity, function(ent)
        GameStatePaused()
    end)
end

return M