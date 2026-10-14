
local M = {}

function M:on_init()
    RegisterButtonCallback(self.entity, function(ent)
        GameStateHowToPlay()
    end)
end

return M