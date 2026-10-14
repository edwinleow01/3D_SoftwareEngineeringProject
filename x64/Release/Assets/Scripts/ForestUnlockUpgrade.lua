-- ForestUnlockUpgrade.lua
local M = {}

function M:on_init()
    print("[ForestUnlock] Initializing for entity:", self.entity)
    
    RegisterButtonCallback(self.entity, function(ent)
        print("[ForestUnlock] Forest unlock button clicked!")
        
        -- Check if already unlocked
        if IsForestUnlocked() then
            print("[ForestUnlock] Forest already unlocked!")
            return
        end
        
        -- Get cost
        local cost = GetForestUnlockCost()
        local currentCurrency = GetCurrency()
        
        print("[ForestUnlock] Cost: " .. cost .. ", Current Currency: " .. currentCurrency)
        
        -- Try to unlock
        local success = UnlockForest()
        
        if success then
            UpdateCurrencyUI()  -- Update display
            -- Sound.Play("event:/SpendMoney")
            print("[ForestUnlock] Forest unlocked!")
            print("[ForestUnlock] Remaining currency: " .. GetCurrency())
            
            -- Optional: Teleport player to forest immediately
            -- TeleportToForest()
        else
            print("[ForestUnlock] Failed to unlock forest!")
            print("[ForestUnlock] Need " .. cost .. ", have " .. currentCurrency)
        end
    end)
end

return M