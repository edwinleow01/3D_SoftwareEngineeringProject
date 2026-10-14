local M = {}

-- GLFW keycodes (you could expose a table from C++ if you like)
local K = {
  W = 87, A = 65, S = 83, D = 68,
  UP = 265, DOWN = 264, LEFT = 263, RIGHT = 262,
  L = 76, F1 = 290, O = 79, P = 80, EQUAL = 61, MINUS = 45
}

function M:on_update(dt)
  -- Toggle ground mode with F1
  if IsKeyDown(K.F1) then
    -- just show example: read current then flip; in practice debounce
    -- (debounce: require keyup detection; simple demo only)
    SetGroundMode(not IsKeyDown(-1)) -- replace with your own stored flag if you want real toggle
  end

  -- Camera WASD
  if IsKeyDown(K.W) then CameraMove("W", dt) end
  if IsKeyDown(K.S) then CameraMove("S", dt) end
  if IsKeyDown(K.A) then CameraMove("A", dt) end
  if IsKeyDown(K.D) then CameraMove("D", dt) end

  -- Lighting nudges
  local step = 5.0 * dt
  if IsKeyDown(K.UP)    then NudgeLight("up",    step) end
  if IsKeyDown(K.DOWN)  then NudgeLight("down",  step) end
  if IsKeyDown(K.LEFT)  then NudgeLight("left",  step) end
  if IsKeyDown(K.RIGHT) then NudgeLight("right", step) end

  -- Toggle main light
  if IsKeyDown(K.L) then ToggleLight() end

  -- HDR / Bloom
  if IsKeyDown(K.O) then SetHDR(  1.0 * dt) end  -- adjust to your taste
  if IsKeyDown(K.P) then SetHDR( -1.0 * dt) end
  if IsKeyDown(K.EQUAL) then SetBloom( 1.0 * dt) end
  if IsKeyDown(K.MINUS) then SetBloom(-1.0 * dt) end
end

return M
