
local M = {}

function M:on_init()
    RegisterButtonCallback(self.entity, function(ent)
        GameStateResume()
    end)
end

return M