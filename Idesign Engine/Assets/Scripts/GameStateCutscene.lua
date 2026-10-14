
local M = {}

function M:on_init()
    RegisterButtonCallback(self.entity, function(ent)
        GameStateCutscene()
    end)
end

return M