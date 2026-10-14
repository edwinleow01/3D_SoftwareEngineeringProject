local M = {}

-- GLFW keycodes (adjust if your IsKeyDown uses different numbers)
local K = {
  G = 71,   -- G key to fake-add points
}

local lastScore = nil

function M:on_init()
  -- Force the first update to rewrite the text
  lastScore = -1

  -- Optional: immediately sync once on init
  local s = GetScore()
  if self.entity then
    SetText(self.entity, "Score: " .. tostring(s))
  end
end

function M:on_update(dt)
  -- 1) TEST BUTTON: press G to fake-add points
  if IsKeyDown(K.G) then
    -- This updates the engine-side score
    AddScore(10)
  end

  -- 2) Read score from engine and update text if changed
  local s = GetScore()

  if s ~= lastScore and self.entity then
    lastScore = s
    SetText(self.entity, "Score: " .. tostring(s))
  end
end

return M
