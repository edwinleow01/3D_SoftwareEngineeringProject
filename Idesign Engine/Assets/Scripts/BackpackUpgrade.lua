-- BackpackUpgrade.lua
local M = {}

function M:on_init()
    print("[BackpackUpgrade] Initializing for entity:", self.entity)
    
    RegisterButtonCallback(self.entity, function(ent)
        print("[BackpackUpgrade] Upgrade button clicked!")
        
        -- Get current currency and cost
        local cost = GetBackpackUpgradeCost()
        local currentCurrency = GetCurrency()
        local currentLevel = GetBackpackLevel()
        
        print("[BackpackUpgrade] Cost: " .. cost .. ", Current Currency: " .. currentCurrency)
        print("[BackpackUpgrade] Current Level: " .. currentLevel)
        
        -- Check if already at max
        if currentLevel >= 3 then
            print("[BackpackUpgrade] Already at max backpack level!")
            return
        end
        
        -- Check if player can afford it
        if currentCurrency >= cost then
            -- Try to upgrade (C++ will handle currency deduction)
            local success = UpgradeBackpack()
            
            if success then
                UpdateCurrencyUI()
                Sound.Play("event:/SpendMoney")
                local newLevel = GetBackpackLevel()
                local newSlots = GetBackpackSlotCount()
                local newCost = GetBackpackUpgradeCost()
                print("[BackpackUpgrade] Upgraded! New level: " .. newLevel)
                print("[BackpackUpgrade] New slots: " .. newSlots)
                print("[BackpackUpgrade] Next upgrade cost: " .. newCost)
                print("[BackpackUpgrade] Remaining currency: " .. GetCurrency())
            else
                print("[BackpackUpgrade] Upgrade failed!")
            end
        else
            print("[BackpackUpgrade] Not enough currency!")
            print("[BackpackUpgrade] Need " .. cost .. ", have " .. currentCurrency)
        end
    end)
end

return M