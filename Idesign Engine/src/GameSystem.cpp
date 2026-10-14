#include "pch.h" 
#include "EventSystem.h"
#include "EventManager.h"
#include "ScriptingSystem.h"
#include "GameSystem.h"
#include <EventReceiverComponent.h>
#include <BoxColliderComponent.h>
#include "Inputhandler.h"
#include <RecipeBookManager.h>
#include "PhaseManager.h"
#include <fstream>                      
#include <rapidjson/document.h>         
#include <rapidjson/istreamwrapper.h>   
#include "ItemComponent.h"
#include "Upgrade.h"
#include <SceneInterface.h>

extern Framework::Coordinator ecsInterface;
Framework::RecipeBookManager* recipeBookManager = nullptr;
Framework::InputHandler* inputHandler = nullptr;

namespace Framework
{
    // Static member initialisation
    std::unordered_map<std::string, int> GameSystem::gameData;
    std::vector<GameSystem::DayStats>    GameSystem::s_dayHistory;
    PhaseManager* phaseManager = nullptr;
    bool GameSystem::m_debugRevealAll = false;

    // =========================================================
    // NIGHT SCALING CONFIG
    // Tweak these to adjust difficulty curve.
    // requiredCustomers = BASE + (night - 1) * PER_NIGHT, capped at MAX
    // =========================================================
    static constexpr int REQUIRED_CUSTOMERS_BASE = 5;   // Night 1: 5 customers required
    static constexpr int REQUIRED_CUSTOMERS_PER_NIGHT = 2;  // +2 per night
    static constexpr int REQUIRED_CUSTOMERS_MAX = 15;  // Hard cap

    void GameSystem::Initialize()
    {
        SetData("ResetMapIconNextOpen", 0);
        std::filesystem::path projectRoot = ScriptingSystem::GetProjectRoot();
        std::string itemsPath = (projectRoot / "Assets/JSON/items.json").string();
        bool success = InitializeItemDatabase(itemsPath);

        std::string unlockPath = (projectRoot / "Assets/JSON/save_data.json").string();
        bool success2 = LoadUnlockedItems(unlockPath);

        if (!success) {
            std::cerr << "CRITICAL WARNING: Items failed to load!" << std::endl;
        }

        if (!success2) {
            std::cerr << "CRITICAL WARNING: Save Data failed to load!" << std::endl;
        }

        if (recipeBookManager == nullptr) {
            recipeBookManager = new RecipeBookManager(&ecsInterface);

            if (InputHandler::GetInstance()) {
                InputHandler::GetInstance()->SetRecipeManager(recipeBookManager);
                std::cout << "[GameSystem] Linked RecipeBookManager to InputHandler." << std::endl;
            }
        }

        if (phaseManager == nullptr) {
            phaseManager = new PhaseManager(&ecsInterface);
            phaseManager->Initialize();
        }

        // =========================================================
        // Initialize default values
        // =========================================================
        SetData("currency", 0);
        SetData("currencyMultiplier", 100);     // 100 = 1.0x
        SetData("customerQueue", 0);        
        SetData("currentNight", 0);             // Night tracking - starts at 0, incremented to 1 on first OnNightStart
        SetData("requiredCustomers", REQUIRED_CUSTOMERS_BASE);  // Required customers - set properly when night starts

        // ---- Per-day accumulators (reset each Day phase) ----
        SetData("dayGoldFromCustomers", 0);
        SetData("dayCustomersServed", 0);
        SetData("dayCustomersSpawned", 0);
        SetData("dayTimeRemainingX100", 0);
        SetData("dayTimeBonusGold", 0);

        // Clear history on full game init
        s_dayHistory.clear();

        // OnDayStart -> reset per-day accumulators
        EventManager::Instance().Subscribe("OnDayStart", [](void*) -> bool {
            GameSystem::ResetDayAccumulators();
            return true;
            });

        // =========================================================
        // Subscribe to OnNightStart to increment night counter
        // and recompute per-night values
        // =========================================================
        EventManager::Instance().Subscribe("OnNightStart", [](void* /*data*/) -> bool {
            // Increment night number
            int night = GameSystem::GetData("currentNight") + 1;
            GameSystem::SetData("currentNight", night);

            // Scale required customers: BASE + (night-1) * PER_NIGHT, capped
            int required = REQUIRED_CUSTOMERS_BASE + (night - 1) * REQUIRED_CUSTOMERS_PER_NIGHT;
            if (required > REQUIRED_CUSTOMERS_MAX) required = REQUIRED_CUSTOMERS_MAX;
            GameSystem::SetData("requiredCustomers", required);

            // Inside OnNightStart callback
            int baseMultiplier = GameSystem::GetData("currencyMultiplier");
            GameSystem::SetData("currencyMultiplier", baseMultiplier);

            return true;
            });

        // IMPORTANT: Initialize UpgradeManager to load saved unlock states
        UpgradeManager::Instance().Initialize();
    }

    GameSystem::~GameSystem()
    {
        Shutdown();
    }

    void GameSystem::Update(float timeslice) {
        static bool wasBookOpen = false;
        if (GameSystem::bookOpen != wasBookOpen) {
            if (GameSystem::bookOpen) {
                std::cout << "\nBook Opened!" << std::endl;
                if (recipeBookManager) recipeBookManager->BuildBookUI();
                g_AudioSystem->PlayAudio("event:/PaperUp");
            }
            else {
                std::cout << "\nBook Closed!" << std::endl;
                recipeBookManager->bookSpawned = false;
                if (recipeBookManager) recipeBookManager->ClearBook();
                g_AudioSystem->PlayAudio("event:/PaperDown");
            }
            wasBookOpen = GameSystem::bookOpen;
        }

        if (GameSystem::bookOpen && recipeBookManager) {
            recipeBookManager->UpdateVisuals(timeslice);
        }
    }

    // =========================================================
    // DATA MANAGEMENT FUNCTIONS
    // =========================================================

    void GameSystem::SetData(const std::string& key, int value)
    {
        gameData[key] = value;
        //if (key.find("ElapsedTime") == std::string::npos)
        //{
        //    RegrowthTimer keeps getting spammed here
        //    std::cout << "[GameSystem] Set " << key << " = " << value << std::endl;
        //}
    }

    void GameSystem::AddData(const std::string& key, int delta)
    {
        if (gameData.find(key) == gameData.end()) {
            gameData[key] = 0;
            std::cout << "[GameSystem] Created new data entry: " << key << " = 0" << std::endl;
        }

        gameData[key] += delta;
        std::cout << "[GameSystem] Added " << delta << " to " << key << ", new value = " << gameData[key] << std::endl;
    }

    int GameSystem::GetData(const std::string& key)
    {
        if (gameData.find(key) == gameData.end()) {
            std::cout << "[GameSystem] Data key '" << key << "' not found, returning 0" << std::endl;
            return 0;
        }
        return gameData[key];
    }

    bool GameSystem::HasData(const std::string& key)
    {
        return gameData.find(key) != gameData.end();
    }

    void GameSystem::ToggleDebugReveal() {
        m_debugRevealAll = !m_debugRevealAll;
        std::cout << "[GameSystem] Debug Reveal All: " << (m_debugRevealAll ? "ON" : "OFF") << std::endl;

        // If the book is open, tell the manager to rebuild the UI with new visibility
        if (GameSystem::bookOpen && recipeBookManager) {
            recipeBookManager->RefreshBook();
        }
    }

    // =========================================================
    // DAY STATS IMPLEMENTATION
    // =========================================================

    void GameSystem::ResetDayAccumulators()
    {
        SetData("dayGoldFromCustomers", 0);
        SetData("dayCustomersServed", 0);
        SetData("dayCustomersSpawned", 0);
        SetData("dayTimeRemainingX100", 0);
        SetData("dayTimeBonusGold", 0);
        SetData("dayCustomersFailed", 0);
        std::cout << "[GameSystem] Day accumulators reset." << std::endl;
    }

    void GameSystem::SaveDayStats(float timeRemaining)
    {
        DayStats day;
        day.dayIndex = sceneInterface.GetCurrentDay();
        day.goldFromCustomers = GetData("dayGoldFromCustomers");
        day.timeRemainingX100 = static_cast<int>(timeRemaining * 100.0f);
        day.timeBonusGold = static_cast<int>(timeRemaining * TIME_BONUS_RATE);        
        
        // Award time bonus to player currency
        if (day.timeBonusGold > 0)
            AddData("currency", day.timeBonusGold);

        day.totalGold = day.goldFromCustomers + day.timeBonusGold;
        day.customersServed = GetData("dayCustomersServed");
        day.customersSpawned = GetData("dayCustomersSpawned");
        int failed = GetData("dayCustomersFailed");

        day.stars = ComputeStars(day.customersServed, day.customersSpawned, day.totalGold, failed);

        s_dayHistory.push_back(day);

        std::cout << "[GameSystem] SaveDayStats"
            << " | Day " << day.dayIndex
            << " | Gold: " << day.totalGold
            << " | Served " << day.customersServed << "/" << day.customersSpawned
            << " | Stars: " << day.stars
            << " | TimeLeft " << timeRemaining << "s\n";
    }

    GameSystem::DayStats GameSystem::GetLastDayStats()
    {
        if (s_dayHistory.empty()) return DayStats{};
        return s_dayHistory.back();
    }

    // Star computation:
    //   servedRatio = customersServed / customersSpawned
    //   goldRatio   = totalGold / EXPECTED_GOLD_PER_DAY  (capped at 1.0)
    //   final ratio = min(servedRatio, goldRatio) — both must be good
    //
    // Tune EXPECTED_GOLD_PER_DAY to match your typical maximum daily earning.
    int GameSystem::ComputeStars(int served, int spawned, int totalGold, int failed)
    {
        static constexpr float EXPECTED_GOLD_PER_DAY = 300.0f;
        float goldRatio = (float)totalGold / EXPECTED_GOLD_PER_DAY;
        if (goldRatio > 1.0f) goldRatio = 1.0f;

        float servedRatio = (spawned > 0) ? (float)served / (float)spawned : goldRatio;

        // Penalise for failures — each failure reduces efficiency
        int totalAttempts = served + failed;
        float efficiency = (totalAttempts > 0) ? (float)served / (float)totalAttempts : 1.0f;

        float ratio = servedRatio * goldRatio * efficiency;

        if (ratio >= 0.70f) return 3;
        if (ratio >= 0.40f) return 2;
        if (ratio >= 0.15f) return 1;
        return 0;
    }

    void GameSystem::Shutdown()
    {
        if (recipeBookManager) {
            recipeBookManager->Shutdown();
            delete recipeBookManager;
            recipeBookManager = nullptr;
        }

        if (phaseManager) {
            delete phaseManager;
            phaseManager = nullptr;
        }

        gameData.clear();
        s_dayHistory.clear();
        std::cout << "[GameSystem] Shutdown complete. Managers freed." << std::endl;
    }
}