-- MapController.lua
-- Helper script for showing/hiding map using EngineState system
-- This can be called from anywhere in your game (e.g., from upgrade screen)

local M = {}

-- Show the map selection screen
-- This uses the EngineState system - no need to manually show/hide entities
function M:ShowMapSelection()
    print("[MapController] ShowMapSelection called")
    
    -- Push MapSelection state - SceneInterface will automatically show all "map" tagged entities
    PushEngineState("MapSelection")
    SetUIFocused(true)
    
    print("[MapController] MapSelection state pushed - map is now visible")
    return true
end

-- Hide the map selection screen
function M:HideMapSelection()
    print("[MapController] HideMapSelection called")
    
    -- Pop MapSelection state - SceneInterface will automatically hide all "map" tagged entities
    PopEngineState()
    SetUIFocused(false)
    
    print("[MapController] MapSelection state popped - map is now hidden")
    return true
end

-- Example usage in your "Next Day" button script:
--[[
function OnNextDayButtonClicked()
    print("[UpgradeScreen] Next Day button clicked")
    
    -- Exit Upgrade state
    PopEngineState()
    
    -- Show loading screen (if you have one)
    -- ShowLoadingScreen()
    -- ... your loading logic ...
    
    -- Show map selection using EngineState
    local MapController = require("MapController")
    MapController:ShowMapSelection()
end
]]

return M