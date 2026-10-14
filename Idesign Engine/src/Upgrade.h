#pragma once
#ifndef _UPGRADE_H_
#define _UPGRADE_H_

#include "Inventory.h"
#include "GameSystem.h"

namespace Framework
{
    enum GameArea
    {
        Home = 0,       // Default starting area
        Forest = 1,     // Unlockable area for mining
        Quarry = 2      // Unlockable area for forging
    };

    class UpgradeManager
    {
    public:
        static UpgradeManager& Instance();

        // Initialize - loads saved unlock states from GameSystem
        void Initialize();

        // Inventory upgrades
        bool UpgradeInventorySlots(int amount);
        int GetInventoryUpgradeCost() const;
        bool CanAffordInventoryUpgrade() const;

        // Currency multiplier upgrades
        bool UpgradeCurrencyMultiplier(float additiveBonus); // e.g., 0.5 for +50%
        float GetCurrencyMultiplier() const;
        int GetCurrencyUpgradeCost() const;
        bool CanAffordCurrencyUpgrade() const;
        int GetCurrencyLevel() const;

        // Area unlock upgrades (Forest only now)
        bool UnlockArea(GameArea area);
        bool IsAreaUnlocked(GameArea area) const;
        int GetAreaUnlockCost(GameArea area) const;

        // Cauldron upgrade (one-time purchase - unlocks 2nd auto-stir cauldron)
        bool UpgradeCauldron();
        bool IsCauldronUnlocked() const;
        int GetCauldronUpgradeCost() const;
        bool CanAffordCauldronUpgrade() const;

        // Backpack upgrades
        bool UpgradeBackpack();
        int GetBackpackUpgradeCost() const;
        bool CanAffordBackpackUpgrade() const;

        // Max level checks for UI
        bool IsInventoryMaxLevel() const;
        bool IsCurrencyMaxLevel() const;
        bool IsBackpackMaxLevel() const;

        // Resets all upgrade state to defaults
        void ResetAll();

        // Max level constants
        static constexpr int MAX_HOTBAR_LEVEL = 1;      // Level 1 = 6 slots (max for hotbar)
        static constexpr int MAX_BACKPACK_LEVEL = 3;    // Level 3 = 3 rows
        static constexpr int MAX_CURRENCY_LEVEL = 3;    // Max level 3 for currency upgrades

    private:
        UpgradeManager() = default;

        // Inventory System Cost
        int m_inventoryUpgradeCost = 600;

        // Currency multiplier system
        float m_currencyMultiplier = 1.0f;  // Start at 1.0x
        int m_currencyUpgradeCost = 300;    // Initial cost

        int m_backpackUpgradeCost = 300;    // Initial cost for backpack upgrade

        int m_currencyLevel = 1;            // Track currency upgrade level (0-3)

        // Cauldron upgrade (one-time)
        bool m_cauldronUnlocked = false;
        int  m_cauldronUpgradeCost = 1000;

        // Area unlock system (Forest only)
        std::unordered_map<GameArea, bool> m_unlockedAreas =
        {
            {GameArea::Home,   true},   // Home is always unlocked
            {GameArea::Forest, true},   // Forest starts unlocked with Home
        };

        std::unordered_map<GameArea, int> m_areaUnlockCosts =
        {
            {GameArea::Forest, 0},
        };
    };
}
#endif // !_UPGRADE_H_