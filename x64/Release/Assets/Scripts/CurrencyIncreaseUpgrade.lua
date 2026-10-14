-- CurrencyIncreaseUpgrade.lua
local M = {}

function M:on_init()
    print("[CurrencyUpgrade] Initializing for entity:", self.entity)
    
    RegisterButtonCallback(self.entity, function(ent)
        print("[CurrencyUpgrade] Currency upgrade button clicked!")
        
        -- Get current level and cost
        local currentLevel = GetCurrencyLevel()
        local cost = GetCurrencyUpgradeCost()
        local currentCurrency = GetCurrency()
        
        print("[CurrencyUpgrade] Current Level: " .. currentLevel)
        print("[CurrencyUpgrade] Cost: " .. cost .. ", Current Currency: " .. currentCurrency)
        
        -- Check if already at max level
        if currentLevel >= 3 then
            print("[CurrencyUpgrade] Already at max currency level!")
            return
        end
        
        -- Try to upgrade - C++ handles everything!
        local success = UpgradeCurrencyMultiplier(0.5)
        
        if success then
            UpdateCurrencyUI()
            Sound.Play("event:/SpendMoney")
            
            local newLevel = GetCurrencyLevel()
            local newMultiplier = GetCurrencyMultiplier()
            local nextCost = GetCurrencyUpgradeCost()
            
            print("[CurrencyUpgrade] Upgraded to Level " .. newLevel .. "!")
            print("[CurrencyUpgrade] New multiplier: " .. newMultiplier .. "x")
            print("[CurrencyUpgrade] Next upgrade cost: " .. nextCost)
            print("[CurrencyUpgrade] Remaining currency: " .. GetCurrency())
        else
            print("[CurrencyUpgrade] Failed to upgrade!")
            print("[CurrencyUpgrade] Need " .. cost .. ", have " .. currentCurrency)
        end
    end)
end

return M