-- Example Lua script usage
local MyScript = {}

function MyScript:on_init()
    -- Get all entities with the "Enemy" tag
    local enemies = GetEntitiesByTag("Enemy")
    
    print("Found " .. #enemies .. " enemies")
    
    -- Loop through all enemies
    for i, entity in ipairs(enemies) do
        print("Enemy entity ID: " .. entity)
        
        -- You can now work with each entity
        local x, y, z = GetPosition(entity)
        print("  Position: " .. x .. ", " .. y .. ", " .. z)
    end
end

function MyScript:on_update(dt)
    -- Find all "Boss" tagged entities and do something with them
    local bosses = GetEntitiesByTag("Boss")
    
    for _, boss in ipairs(bosses) do
        -- Do boss-specific logic
    end
end

return MyScript