local M = {}

function M:on_init()
    print("[GameStateStart] on_init called for entity:", self.entity)


    RegisterButtonCallback(self.entity, function(ent)
        print("[GameStateStart] Play button clicked! Entity:", ent)
        print("[GameStateStart] ==================== GAME START SEQUENCE ====================")

        

        -- Step 1: Transition from Menu to Play state
        print("[GameStateStart] Step 1: Transitioning from Menu to Play state...")
        PopEngineState()  -- Pop Menu state
        PushEngineState("Play")  -- Push Play state
        
        -- Step 2: Start the game state (your game-specific initialization)
        print("[GameStateStart] Step 2: Starting game state...")
        GameStateStart()


        
        -- Step 4: Show map selection UI using EngineState
        print("[GameStateStart] Step 4: Showing map selection via EngineState...")
        
        -- Push MapSelection state - this will automatically show all entities tagged with "map"
        PushEngineState("MapSelection")
        SetUIFocused(true)
        
        print("[GameStateStart] MapSelection state pushed - map should now be visible")
        
        -- Optional: Call ShowMap on MapSelectionUI script if additional logic is needed
        local mapUI = FindEntityByName("MapSelectionUI")
        if mapUI and IsEntityValid(mapUI) then
            print("[GameStateStart] Found MapSelectionUI (ID: " .. mapUI .. ")")
            -- The map is already visible via EngineState, but we can call ShowMap for any additional setup
            local success, err = pcall(function()
                CallLuaFunction(mapUI, "ShowMap")
            end)
            
            if success then
                print("[GameStateStart] Called ShowMap on MapSelectionUI for additional setup")
            else
                print("[GameStateStart] Note: " .. tostring(err))
            end
        else
            print("[GameStateStart] Warning: MapSelectionUI entity not found, but map should still be visible via EngineState")
        end
        
        print("[GameStateStart] ==================== END GAME START SEQUENCE ====================")
    end)
    
    print("[GameStateStart] Button callback registered successfully")
end

return M