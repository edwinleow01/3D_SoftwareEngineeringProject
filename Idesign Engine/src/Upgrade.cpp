#include "pch.h"
#include "Upgrade.h"

namespace Framework
{
    UpgradeManager& UpgradeManager::Instance()
    {
        static UpgradeManager instance;
        return instance;
    }

    // =========================================================
    // INITIALIZATION - Load saved states
    // =========================================================
    void UpgradeManager::Initialize()
    {
        std::cout << "[UpgradeManager] Initializing..." << std::endl;

        // Check Forest unlock state (starts unlocked, but load saved state)
        if (GameSystem::HasData("area_1")) {
            int forestUnlocked = GameSystem::GetData("area_1");
            m_unlockedAreas[GameArea::Forest] = (forestUnlocked == 1);
            std::cout << "[UpgradeManager] Loaded Forest unlock state: " << (forestUnlocked == 1 ? "UNLOCKED" : "LOCKED") << std::endl;
        }
        else {
            GameSystem::SetData("area_1", 1);  // 1 = unlocked
            std::cout << "[UpgradeManager] Forest default state: UNLOCKED" << std::endl;
        }

        // Check Cauldron upgrade state
        if (GameSystem::HasData("cauldron_unlocked")) {
            m_cauldronUnlocked = (GameSystem::GetData("cauldron_unlocked") == 1);
            std::cout << "[UpgradeManager] Loaded Cauldron state: " << (m_cauldronUnlocked ? "UNLOCKED" : "LOCKED") << std::endl;
        }
        else {
            GameSystem::SetData("cauldron_unlocked", 0);  // 0 = locked
            std::cout << "[UpgradeManager] Cauldron default state: LOCKED" << std::endl;
        }

        // Load currency multiplier
        if (GameSystem::HasData("currencyMultiplier")) {
            int multiplierAsInt = GameSystem::GetData("currencyMultiplier");
            m_currencyMultiplier = multiplierAsInt / 100.0f;
            std::cout << "[UpgradeManager] Loaded currency multiplier: " << m_currencyMultiplier << "x" << std::endl;
        }

        // Load currency level
        if (GameSystem::HasData("currencyLevel")) {
            m_currencyLevel = GameSystem::GetData("currencyLevel");
            std::cout << "[UpgradeManager] Loaded currency level: " << m_currencyLevel << std::endl;
        }

        std::cout << "[UpgradeManager] Initialization complete!" << std::endl;
    }

    // =========================================================
    // INVENTORY UPGRADES
    // =========================================================
    bool UpgradeManager::CanAffordInventoryUpgrade() const
    {
        int currentCurrency = GameSystem::GetCurrency();
        return currentCurrency >= m_inventoryUpgradeCost;
    }

    bool UpgradeManager::UpgradeInventorySlots(int amount)
    {
        auto& inv = Inventory::Instance();

        // Check if player can afford it
        if (!CanAffordInventoryUpgrade())
        {
            std::cout << "[UpgradeManager] Not enough currency! Need " << m_inventoryUpgradeCost
                << ", have " << GameSystem::GetCurrency() << std::endl;
            return false;
        }

        // Deduct the cost
        GameSystem::AddCurrency(-m_inventoryUpgradeCost);
        std::cout << "[UpgradeManager] Deducted " << m_inventoryUpgradeCost << " currency" << std::endl;

        // Apply the upgrade
        //inv.UpgradeSlots(amount);

        // Cost stays the same (multiplier disabled)
        //m_inventoryUpgradeCost += 50;

        std::cout << "[UpgradeManager] Inventory upgraded! New slots: " << inv.GetHotbarSlotCount()
            << ", Next cost: " << m_inventoryUpgradeCost << std::endl;

        return true;
    }

    int UpgradeManager::GetInventoryUpgradeCost() const
    {
        return m_inventoryUpgradeCost;
    }

    // =========================================================
    // CURRENCY MULTIPLIER UPGRADES
    // =========================================================
    bool UpgradeManager::CanAffordCurrencyUpgrade() const
    {
        int currentCurrency = GameSystem::GetCurrency();
        return currentCurrency >= m_currencyUpgradeCost;
    }

    bool UpgradeManager::UpgradeCurrencyMultiplier(float additiveBonus)
    {
        // Check if already at max level
        if (m_currencyLevel >= MAX_CURRENCY_LEVEL)
        {
            std::cout << "[UpgradeManager] Currency already at max level!" << std::endl;
            return false;
        }

        // Check if player can afford it
        if (!CanAffordCurrencyUpgrade())
        {
            std::cout << "[UpgradeManager] Not enough currency! Need " << m_currencyUpgradeCost
                << ", have " << GameSystem::GetCurrency() << std::endl;
            return false;
        }

        // Deduct the cost
        GameSystem::AddCurrency(-m_currencyUpgradeCost);
        std::cout << "[UpgradeManager] Deducted " << m_currencyUpgradeCost << " currency" << std::endl;

        // Apply the upgrade
        m_currencyMultiplier += additiveBonus;
        m_currencyLevel++;  // Increment level

        // Store as integer in GameSystem (100 = 1.0x)
        int multiplierAsInt = static_cast<int>(m_currencyMultiplier * 100);
        GameSystem::SetData("currencyMultiplier", multiplierAsInt);
        GameSystem::SetData("currencyLevel", m_currencyLevel);  // Save level too

        std::cout << "[UpgradeManager] Currency multiplier upgraded to "
            << m_currencyMultiplier << "x (Level " << m_currencyLevel << ")" << std::endl;

        // Cost stays the same (multiplier disabled)
        //m_currencyUpgradeCost += 100;

        std::cout << "[UpgradeManager] Next currency upgrade cost: " << m_currencyUpgradeCost << std::endl;

        return true;
    }

    float UpgradeManager::GetCurrencyMultiplier() const
    {
        return m_currencyMultiplier;
    }

    int UpgradeManager::GetCurrencyUpgradeCost() const
    {
        return m_currencyUpgradeCost;
    }

    int UpgradeManager::GetCurrencyLevel() const
    {
        return m_currencyLevel;
    }

    bool UpgradeManager::UnlockArea(GameArea area)
    {
        if (IsAreaUnlocked(area))
        {
            std::cout << "[UpgradeManager] Area already unlocked!" << std::endl;
            return false;
        }

        int cost = GetAreaUnlockCost(area);
        int currentCurrency = GameSystem::GetCurrency();

        if (currentCurrency < cost)
        {
            std::cout << "[UpgradeManager] Not enough currency! Need " << cost
                << ", have " << currentCurrency << std::endl;
            return false;
        }

        GameSystem::AddCurrency(-cost);
        m_unlockedAreas[area] = true;

        std::string areaKey = "area_" + std::to_string(static_cast<int>(area));
        GameSystem::SetData(areaKey, 1);

        std::cout << "[UpgradeManager] Area unlocked! Remaining currency: "
            << GameSystem::GetCurrency() << std::endl;

        return true;
    }

    bool UpgradeManager::IsAreaUnlocked(GameArea area) const
    {
        auto it = m_unlockedAreas.find(area);
        if (it != m_unlockedAreas.end())
            return it->second;
        return false;
    }

    int UpgradeManager::GetAreaUnlockCost(GameArea area) const
    {
        auto it = m_areaUnlockCosts.find(area);
        if (it != m_areaUnlockCosts.end())
            return it->second;
        return 0;
    }

    // =========================================================
    // CAULDRON UPGRADE (one-time - unlocks 2nd auto-stir cauldron)
    // =========================================================
    bool UpgradeManager::CanAffordCauldronUpgrade() const
    {
        return GameSystem::GetCurrency() >= m_cauldronUpgradeCost;
    }

    bool UpgradeManager::UpgradeCauldron()
    {
        if (m_cauldronUnlocked)
        {
            std::cout << "[UpgradeManager] Cauldron already unlocked!" << std::endl;
            return false;
        }

        if (!CanAffordCauldronUpgrade())
        {
            std::cout << "[UpgradeManager] Not enough currency! Need " << m_cauldronUpgradeCost
                << ", have " << GameSystem::GetCurrency() << std::endl;
            return false;
        }

        GameSystem::AddCurrency(-m_cauldronUpgradeCost);
        m_cauldronUnlocked = true;
        GameSystem::SetData("cauldron_unlocked", 1);

        std::cout << "[UpgradeManager] Cauldron unlocked! Remaining currency: "
            << GameSystem::GetCurrency() << std::endl;

        return true;
    }

    bool UpgradeManager::IsCauldronUnlocked() const
    {
        return m_cauldronUnlocked;
    }

    int UpgradeManager::GetCauldronUpgradeCost() const
    {
        return m_cauldronUpgradeCost;
    }

    bool UpgradeManager::CanAffordBackpackUpgrade() const
    {
        int currentCurrency = GameSystem::GetCurrency();
        return currentCurrency >= m_backpackUpgradeCost;
    }

    bool UpgradeManager::UpgradeBackpack()
    {
        auto& inv = Inventory::Instance();
        int currentLevel = inv.GetBackpackLevel();

        // Check if already at max
        if (currentLevel >= 3)
        {
            std::cout << "[UpgradeManager] Backpack already at max level!" << std::endl;
            return false;
        }

        // Check if player can afford it
        if (!CanAffordBackpackUpgrade())
        {
            std::cout << "[UpgradeManager] Not enough currency! Need " << m_backpackUpgradeCost
                << ", have " << GameSystem::GetCurrency() << std::endl;
            return false;
        }

        // Deduct the cost
        GameSystem::AddCurrency(-m_backpackUpgradeCost);
        std::cout << "[UpgradeManager] Deducted " << m_backpackUpgradeCost << " currency" << std::endl;

        // Apply the upgrade
        inv.SetBackpackLevel(currentLevel + 1);

        // Cost stays the same (multiplier disabled)
        //m_backpackUpgradeCost += 150;

        std::cout << "[UpgradeManager] Backpack upgraded to level " << (currentLevel + 1)
            << "! Next cost: " << m_backpackUpgradeCost << std::endl;

        return true;
    }

    int UpgradeManager::GetBackpackUpgradeCost() const
    {
        return m_backpackUpgradeCost;
    }

    // =========================================================
    // MAX LEVEL CHECKS (for UI completion stamps)
    // =========================================================
    bool UpgradeManager::IsInventoryMaxLevel() const
    {
        auto& inv = Inventory::Instance();
        return inv.GetHotbarLevel() >= MAX_HOTBAR_LEVEL;
    }

    bool UpgradeManager::IsCurrencyMaxLevel() const
    {
        return m_currencyLevel >= MAX_CURRENCY_LEVEL;
    }

    bool UpgradeManager::IsBackpackMaxLevel() const
    {
        auto& inv = Inventory::Instance();
        return inv.GetBackpackLevel() >= MAX_BACKPACK_LEVEL;
    }

    // =========================================================
    // RESET ALL
    // =========================================================
    void UpgradeManager::ResetAll()
    {
        m_currencyMultiplier = 1.0f;
        m_currencyLevel = 1;
        m_cauldronUnlocked = false;

        // Reset GameSystem data keys so Initialize() loads clean state next run
        GameSystem::SetData("currencyMultiplier", 100); // 100 = 1.0x
        GameSystem::SetData("currencyLevel", 1);
        GameSystem::SetData("cauldron_unlocked", 0);

        // Forest stays unlocked (default start state)
        m_unlockedAreas[GameArea::Forest] = true;
        GameSystem::SetData("area_1", 1);

        Inventory::Instance().ResetBackpackLevel(); // locked
        //Inventory::Instance().SetHotbarLevel(1);

        // Reset inventory items
        Inventory::Instance().ClearAll();

        std::cout << "[UpgradeManager] ResetAll: all upgrades reset to defaults.\n";
    }
}