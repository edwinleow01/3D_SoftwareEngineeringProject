 // --- PhaseManager.h ---
#include "pch.h"
#include <glm/glm/glm.hpp>
#include <string>
#include "Coordinator.h"
#include "Upgrade.h"
namespace Framework {

    enum class GamePhase
    {
        GameStart,
        Day,
        Night,
        Upgrade
    };

    class PhaseManager
    {
    public:
        PhaseManager(Coordinator* coord) : coordinator(coord) {}

        void Initialize();

        void SetPhase(GamePhase newPhase);
        void TogglePhase();

        // NEW: Handles the delayed pause for the Upgrade screen
        void ApplyUpgradeState();

        GamePhase GetCurrentPhase() const { return m_currentPhase; }

        void StartGame() { SetPhase(GamePhase::Day); }
        void SetDaySpawn(const glm::vec3& pos, const glm::vec3& rot) { m_daySpawnPos = pos; m_daySpawnRot = rot; }
        void SetNightSpawn(const glm::vec3& pos, const glm::vec3& rot) { m_nightSpawnPos = pos; m_nightSpawnRot = rot; }

        glm::vec3& GetDaySpawnPosRef() { return m_daySpawnPos; }
        glm::vec3& GetNightSpawnPosRef() { return m_nightSpawnPos; }

        void TeleportPlayer(const glm::vec3& position, const glm::vec3& rotation);
        bool dayTutorial = false;
        bool nightTutorial = false;
        bool upgradeTutorial = false;
    private:
        Coordinator* coordinator;
        void HandleEndPhaseTransition(void* data);

        // Trackers to ensure tutorials only play once
        bool m_nightTutorialTriggered = false;
        bool m_dayTutorialTriggered = false;

        GamePhase m_currentPhase = GamePhase::Day;

        glm::vec3 m_daySpawnPos{ -282, 315,  619 };
        glm::vec3 m_daySpawnRot{ 0.0f, 0.0f, 0.0f };

        glm::vec3 m_nightSpawnPos{ -36.0f, 9.0f, 9.5f };
        glm::vec3 m_nightSpawnRot{ 0.0f, 0.0f, 0.0f };

        // Temp: Area-specific teleport points
        glm::vec3 m_forestSpawnPos = glm::vec3(-282.0f, 315.0f, 619.0f);
        glm::vec3 m_forestSpawnRot = glm::vec3(90.0f, 90.0f, 0.0f);

        glm::vec3 m_quarrySpawnPos = glm::vec3(200.0f, 0.0f, 0.0f);
        glm::vec3 m_quarrySpawnRot = glm::vec3(0.0f, 180.0f, 0.0f);
    };

    extern Framework::PhaseManager* phaseManager;
}