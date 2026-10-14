#pragma once
#include "Coordinator.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace Framework 
{
    // Inherit from ISystem (the interface you shared)
    class GameSystem : public ISystem 
    {
    public:

        // =========================================================
        // DAY STATS — one entry per completed day
        // =========================================================
        struct DayStats
        {
            int dayIndex = 0;   // 1-based
            int goldFromCustomers = 0;   // post-multiplier gold from fulfilled orders
            int timeBonusGold = 0;   // remaining_seconds * TIME_BONUS_RATE
            int totalGold = 0;   // goldFromCustomers + timeBonusGold
            int customersServed = 0;   // orders successfully fulfilled
            int customersSpawned = 0;   // total orders queued this night
            int timeRemainingX100 = 0;   // seconds * 100 (int-safe storage)
            int stars = 0;   // 0 / 1 / 2 / 3
        };

        ~GameSystem();

        static bool bookOpen;
        static bool bookOpened;
        static bool m_debugRevealAll; 

        void Initialize() override;
        void Update(float timeslice) override;
        std::string GetName() override { return "GameSystem"; }
        void Shutdown();

        static void ToggleDebugReveal();

        // NEW: Flexible data storage
        static std::unordered_map<std::string, int> gameData;

        // Data management functions
        static void SetData(const std::string& key, int value);
        static void AddData(const std::string& key, int delta);
        static int GetData(const std::string& key);
        static bool HasData(const std::string& key);

        // Currency functions (uses gameData internally)
        static void SetCurrency(int value) { SetData("currency", value); }

        // In GameSystem.cpp
        static void AddCurrency(int delta)
        {
            int finalAmount = delta;

            // 1. Is this income? (Positive delta)
            if (delta > 0)
            {
                int mInt = GetData("currencyMultiplier");
                if (mInt < 100) mInt = 100; // Safety floor
                float multiplier = mInt / 100.0f;

                finalAmount = static_cast<int>(delta * multiplier);

                // Log income details
                std::cout << "\n[GameSystem] Adding currency with multiplier: " << mInt << "%" << std::endl;
                std::cout << "[C++ Math] " << delta << " * " << multiplier << " = " << finalAmount << std::endl;

                // Accumulate for today's stats 
                AddData("dayGoldFromCustomers", finalAmount);
            }
            // 2. Is this an expense? (Negative delta)
            else if (delta < 0)
            {
                // No multiplier is calculated or applied.
                std::cout << "\n[GameSystem] Deducting currency directly: " << finalAmount << std::endl;
            }

            // 3. Apply the final data to the bank
            AddData("currency", finalAmount);
        }

        static int GetCurrency() { return GetData("currency"); }

        // Customer queue functions (now use gameData internally)
        static void SetCustomerQueue(int value) { SetData("customerQueue", value); }
        static void AddCustomerQueue(int delta) { AddData("customerQueue", delta); }
        static int GetCustomerQueue() { return GetData("customerQueue"); }

        // =========================================================
        // DAY STATS API
        // =========================================================

        // Called by PhaseManager when Day -> Night fires.
        // timeRemaining: seconds left on the day timer (0 if natural expiry).
        static void SaveDayStats(float timeRemaining);

        // Resets per-day accumulators. Subscribed to OnDayStart event.
        static void ResetDayAccumulators();

        // Returns a copy of the last completed day's stats.
        static DayStats GetLastDayStats();

        // Returns all day history (read-only).
        static const std::vector<DayStats>& GetDayHistory() { return s_dayHistory; }

        // Gold earned per second of remaining day time. Tune freely.
        static constexpr float TIME_BONUS_RATE = 1.0f;

    private:
        static std::vector<DayStats> s_dayHistory;
        static int ComputeStars(int served, int spawned, int totalGold, int failed);
    };
}