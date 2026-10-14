-- assets/scripts/prove_lua.lua
local M = {}
function M:on_init()
  print("[Lua] on_init OK")
end
function M:on_update(dt)
  -- unmistakable effect with NO key input:
  NudgeLight("right", 1.5 * dt)   -- light drifts right
  SetHDR(2.0)                      -- pin HDR to 2.0 (visible)
end
return M