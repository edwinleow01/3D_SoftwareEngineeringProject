
#pragma once
#ifndef _SCENE_INTERFACE_H_
#define _SCENE_INTERFACE_H_

#include "System.h"
#include "EngineState.h"
#include "GameSystem.h"
#include "Upgrade.h"
namespace Framework 
{
	class SceneInterface : public ISystem
	{
	public:
		void Initialize() override;
		void Update(float deltaTime) override;
		std::string GetName() override;

		static int g_CurrentDay;

		static int GetCurrentDay();

		void SetCurrentDay(int day);

		static void AdvanceToNextDay();

		static void PushDayTransitionStateForCurrentDay();

		void OnNextDayPressed();

		static bool WinOrLose();

	private:
	};

	static float Saturate(float x) { return glm::clamp(x, 0.0f, 1.0f); }

	//int GetTargetDayFromState()
	//{
	//	if (engineState.Is(EngineMode::Day2)) return 2;
	//	if (engineState.Is(EngineMode::Day3)) return 3;
	//	if (engineState.Is(EngineMode::Day4)) return 4;
	//	if (engineState.Is(EngineMode::Day5)) return 5;
	//	return 0;
	//}

	// Declare the global instance of EngineState
	extern SceneInterface sceneInterface;
}
#endif // !_SCENE_INTERFACE_H_