-- Pathfinding Lua script
-- This script demonstrates how to call PathfindTo between two named entities
-- using the exposed FindEntityByName() engine function.

local M = {}

local didPath = false  -- ensure we only call pathfinding once

function M:on_init()
    print("[Lua] Pathfinding test on_init")
end

function M:on_update(dt)
    if not didPath then
        -- Test: find both entities by name
        local agent = FindEntityByName("Customer")
        local target = FindEntityByName("CustomerWaitingPoint")

        if agent == nil then
            print("[Lua] Entity 'Customer' not found!")
            return
        end
        if target == nil then
            print("[Lua] Entity 'CustomerWaitingPoint' not found!")
            return
        end

        print("[Lua] Found entities: Customer = " .. tostring(agent) .. ", CustomerWaitingPoint = " .. tostring(target))

        -- Pathfind from Frog to Target
        PathfindTo("Customer", "CustomerWaitingPoint")
        print("[Lua] Pathfinding triggered once.")

        didPath = true
    end
end

return M
