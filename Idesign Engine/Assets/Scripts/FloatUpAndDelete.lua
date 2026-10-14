-- FloatUpAndDelete.lua
-- This script makes the attached entity float up and delete itself (Text and logs popups)

local FloatUpAndDelete = {}

-- Configuration
local float_speed = 0.05
local lifetime = 2.0

-- Pop-up animation settings (more subtle)
local popup_duration = 0.2       -- Slightly faster
local popup_scale_start = 1.0     
local popup_scale_final = 1.0     -- End at 100% (no overshoot, smoother)

-- Fade-out settings
local fade_start_time = 1.5       -- When to start fading (1.5 seconds)

-- Expected prefab scale
local prefab_base_scale = 1.0     -- The prefab's intended scale

-- Instance-specific state storage (keyed by entity ID)
local instances = {}

-- Helper function to ensure state exists
local function EnsureState(entity)
    if not instances[entity] then
        print("[FloatUpAndDelete] Creating state for entity: " .. tostring(entity))
        
        local x, y, z = GetPosition(entity)

        -- Capture the engine state active at spawn time so we can pause
        -- the animation if the player switches screens mid-float
        local spawnState = "Play"
        if GetCurrentEngineState then
            spawnState = GetCurrentEngineState()
        end
        
        instances[entity] = {
            elapsed_time = 0.0,
            initial_y = y,
            is_destroyed = false,
            spawn_state = spawnState,   -- Engine state this log was created in
        }
        
        -- Start at 70% of the prefab scale
        SetTextComponent(entity, "scale", prefab_base_scale * popup_scale_start)
    end
    return instances[entity]
end

function FloatUpAndDelete.on_init(self)
    print("[FloatUpAndDelete] Initialized for entity: " .. tostring(self.entity))
    
    -- Ensure state is created
    EnsureState(self.entity)
end

function FloatUpAndDelete.on_update(self, dt)
    -- Get or create instance-specific state
    local state = EnsureState(self.entity)
    
    -- GUARD: Don't run if already destroyed
    if state.is_destroyed then
        return
    end

    -- GUARD: Pause animation if we've left the state this log was spawned in.
    -- Hides the log while in a different screen; resumes when back in the same state.
    if GetCurrentEngineState then
        local currentState = GetCurrentEngineState()
        if currentState ~= state.spawn_state then
            SetTextComponent(self.entity, "opacity", 0.0)
            return
        end
    end
    
    state.elapsed_time = state.elapsed_time + dt
    
    -- Pop-up scale animation (smooth ease-out, no bounce)
    if state.elapsed_time < popup_duration then
        local progress = state.elapsed_time / popup_duration
        
        -- Smooth ease-out (no overshoot)
        local t = 1.0 - (1.0 - progress) * (1.0 - progress)  -- Quadratic ease-out
        local scale_factor = popup_scale_start + (popup_scale_final - popup_scale_start) * t
        
        -- Update text scale
        SetTextComponent(self.entity, "scale", prefab_base_scale * scale_factor)
    end
    
    -- Fade-out animation (starts at fade_start_time)
    if state.elapsed_time >= fade_start_time then
        local fade_duration = lifetime - fade_start_time
        local fade_progress = (state.elapsed_time - fade_start_time) / fade_duration
        local opacity = 1.0 - fade_progress  -- 1.0 -> 0.0
        
        -- Clamp opacity
        if opacity < 0.0 then opacity = 0.0 end
        
        -- Update text opacity
        SetTextComponent(self.entity, "opacity", opacity)
    else
        -- Ensure opacity is fully visible before fade begins
        -- (needed to restore visibility after returning from another state)
        SetTextComponent(self.entity, "opacity", 1.0)
    end
    
    -- Get current position
    local x, y, z = GetPosition(self.entity)
    
    -- Move upward
    local new_y = state.initial_y + (float_speed * state.elapsed_time)
    SetPosition(self.entity, x, new_y, z)
    
    -- Destroy when lifetime is reached
    if state.elapsed_time >= lifetime then
        print("[FloatUpAndDelete] Lifetime reached, destroying entity: " .. self.entity)
        state.is_destroyed = true
        
        -- Clean up state
        instances[self.entity] = nil
        
        DestroyEntity(self.entity)
        return
    end
end

return FloatUpAndDelete