
local M = {}

function M:on_init()
    RegisterButtonCallback(self.entity, function(ent)
        GameStateCredit()
    end)
end

return M