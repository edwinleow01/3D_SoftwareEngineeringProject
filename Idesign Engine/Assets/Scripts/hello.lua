local M = {}

-- GLFW keycodes (you can move this table somewhere global later)
local K = {
  F = 70   -- F key
}

function M:on_init()
  print("[Lua] hello.lua on_init")
end

function M:on_update(dt)
  -- Only trigger the message if key F is pressed
  if IsKeyDown(K.F) then
    self:on_event("F key pressed!")
  end
end

function M:on_event(name)
  print("What's up my niglets! " .. tostring(name))
end

return M
