
local M = {}

function M:on_init()
    RegisterButtonCallback(self.entity, function(ent)
        GameStateSettingsAudio()
    end)
end

return M