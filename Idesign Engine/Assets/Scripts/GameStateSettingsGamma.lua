
local M = {}

function M:on_init()
    RegisterButtonCallback(self.entity, function(ent)
        GameStateSettingsGamma()
    end)
end

return M
