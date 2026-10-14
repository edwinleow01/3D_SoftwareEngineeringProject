local M = {}

-- Mouse button codes (assuming GLFW-like mapping)
local MOUSE = {
  LEFT = 0,
  RIGHT = 1,
  MIDDLE = 2
}

-- This variable tracks the mouse state so we only print once per click
local is_mouse_already_down = false

function M:on_init()
  print("[Lua] LeftClickScript on_init")
  is_mouse_already_down = false
end

function M:on_update(dt)
  -- Check if the left mouse button is held down
  -- (Assuming your C++ function is IsMouseButtonDown)
  if IsMouseButtonDown(MOUSE.LEFT) then
    
    -- Check if this is the *first frame* the mouse is down
    if not is_mouse_already_down then
      
      -- *** THIS IS THE NEW PART ***
      -- Instead of printing, we tell the engine to
      -- invoke the "OnClick" event on this entity.
      print("Left Click detected, invoking 'OnClick' event...")
      InvokeEvent("OnClick")
      
      -- Set the flag so we don't fire again until the button is released
      is_mouse_already_down = true
    end
    
  else
    -- The mouse button is not down, so reset the flag
    is_mouse_already_down = false
  end
end

function M:on_event(name)
  print("[Lua] LeftClickScript received event: " .. tostring(name))
end

return M