-- scripts/ui/load_game_scene.lua
local M = {}

function M:on_init()
    -- Bind this entity's button click to scene load
    RegisterButtonCallback(self.entity, function(ent)
        print("[Lua] Button entity", ent, "clicked – loading GameScene.json")

        -- Path should match how you store scenes; adjust if needed
        LoadScene("Scenes/GameScene.json", false)
        -- or just: LoadScene("GameScene.json")
    end)
end

return M
