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
        
        -- Test adding a tag
        AddTagToEntity(entity, "Active")
        print("  Added 'Active' tag to enemy " .. entity)
        
        -- Test if entity has the tag
        if EntityHasTag(entity, "Active") then
            print("  Confirmed: Enemy has 'Active' tag")
        end
        
        -- Add multiple tags for testing
        AddTagToEntity(entity, "Hostile")
        AddTagToEntity(entity, "Spawned")
    end
    
    -- Test finding entities by the newly added tag
    local activeEntities = GetEntitiesByTag("Active")
    print("\nFound " .. #activeEntities .. " entities with 'Active' tag after adding it")
end

function MyScript:on_update(dt)
    -- Find all "Boss" tagged entities and do something with them
    local bosses = GetEntitiesByTag("Boss")
    
    for _, boss in ipairs(bosses) do
        -- Check if boss has certain tags
        if EntityHasTag(boss, "Enraged") then
            print("Boss " .. boss .. " is enraged!")
        else
            -- Add "Enraged" tag after some condition
            AddTagToEntity(boss, "Enraged")
            print("Boss " .. boss .. " is now enraged!")
        end
    end
    
    -- Test removing tags from all "Active" enemies after some time
    -- (You'd normally have a timer here)
    local activeEnemies = GetEntitiesByTag("Active")
    for _, enemy in ipairs(activeEnemies) do
        -- Remove the "Active" tag
        RemoveTagFromEntity(enemy, "Active")
        print("Removed 'Active' tag from enemy " .. enemy)
        
        -- Verify removal
        if not EntityHasTag(enemy, "Active") then
            print("  Confirmed: 'Active' tag removed")
        end
    end
end

return MyScript
