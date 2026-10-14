local PhaseSwitcher = {}

-- This function can be called when the player interacts with a specific entity
function PhaseSwitcher.OnInteract(entityID)

    -- Call the C++ function we just exposed
    TogglePhase()
    
    return true
end

-- Example: Automatic swap if a certain condition is met (checked in Update)
function PhaseSwitcher.OnUpdate(entityID, dt)
    -- You could check for a specific key press here if you want to cheat/debug
    -- if IsKeyDown(75) then -- 'K' key
    --    TogglePhase()
    -- end
end

return PhaseSwitcher