-- Example script: Find entity by name and print its position
-- Make sure GetComponentTransform(entity) is exposed to Lua and returns { x, y, z }

local M = {}



function M:on_update(dt)

    local e = FindEntityByName("Customer")
    if e then
        local x, y, z = GetPosition(e)
        print(string.format("[Lua] Frog found at position: (%.2f, %.2f, %.2f)", x, y, z))
    else
        print("[Lua] Could not find entity named 'Frog'")
    end


end

return M
