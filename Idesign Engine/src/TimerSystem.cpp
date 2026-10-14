///////////////////////////////////////////////////////////////////////////////
///
/// @file TimerSystem.cpp
/// 
/// @brief Implementation of the TimerSystem class, which manages all timer
///        components within the game engine. Iterates entities with a
///        TimerComponent each frame and drives their countdown logic.
///  
///////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "Coordinator.h"
#include "TimerSystem.h"

extern Framework::Coordinator ecsInterface;
namespace Framework
{
    void TimerSystem::Initialize()
    {
        // Nothing to initialise — timers are started via game logic
    }

    void TimerSystem::Update(float deltaTime)
    {
        for (auto entity : mEntities)
        {
            if (!ecsInterface.HasComponent<TimerComponent>(entity))
                continue;

            auto& timer = ecsInterface.GetComponent<TimerComponent>(entity);
            timer.Update(deltaTime);
        }
    }
}