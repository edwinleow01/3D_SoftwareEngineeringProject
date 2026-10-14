-- CauldronUpgrade.lua
local M = {}

function M:on_init()
    print("[CauldronUpgrade] Initializing for entity:", self.entity)

    RegisterButtonCallback(self.entity, function(ent)
        print("[CauldronUpgrade] Cauldron upgrade button clicked!")

        -- Check if already unlocked
        if IsCauldronUnlocked() then
            print("[CauldronUpgrade] Cauldron already unlocked!")
            return
        end

        -- Get cost
        local cost = GetCauldronUpgradeCost()
        local currentCurrency = GetCurrency()

        print("[CauldronUpgrade] Cost: " .. cost .. ", Current Currency: " .. currentCurrency)

        -- Try to upgrade
        local success = UpgradeCauldron()

        if success then
            UpdateCurrencyUI()
            Sound.Play("event:/SpendMoney")
            print("[CauldronUpgrade] Cauldron unlocked!")
            print("[CauldronUpgrade] Remaining currency: " .. GetCurrency())
        else
            print("[CauldronUpgrade] Failed! Need " .. cost .. ", have " .. currentCurrency)
        end
    end)
end

return M