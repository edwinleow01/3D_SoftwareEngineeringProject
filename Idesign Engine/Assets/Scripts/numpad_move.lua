local M = {}

-- GLFW key codes for numpad: adjust if your keymap is different
local K = {
  NP8 = 328,
  NP4 = 324,
  NP5 = 325,
  NP6 = 326,
}

function M:on_update(dt)
  local e = self.entity
  if not e then return end

  -- fetch current position from engine
  local x, y, z = GetPosition(e)

  local speed = 5.0

  if IsKeyDown(K.NP8) then
    z = z - speed * dt   -- forward
  end
  if IsKeyDown(K.NP5) then
    z = z + speed * dt   -- back
  end
  if IsKeyDown(K.NP4) then
    x = x - speed * dt   -- left
  end
  if IsKeyDown(K.NP6) then
    x = x + speed * dt   -- right
  end

  SetPosition(e, x, y, z)
end

return M
