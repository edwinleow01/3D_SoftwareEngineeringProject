///////////////////////////////////////////////////////////////////////////////
///
/// @file TimerSystem.h
/// 
/// @brief Implementation of the TimerSystem class, which manages all timer
///        components within the game engine. Iterates entities with a
///        TimerComponent each frame and drives their countdown logic.
///  
///////////////////////////////////////////////////////////////////////////////
#pragma once
#ifndef _TIMER_SYSTEM_H_
#define _TIMER_SYSTEM_H_

#include "System.h"
#include "ComponentList.h"
#include "TimerComponent.h"

namespace Framework
{
    class TimerSystem : public ISystem
    {
    public:
        TimerSystem() = default;

        void Initialize() override;
        void Update(float deltaTime) override;
        std::string GetName() override { return "TimerSystem"; }
    };
}
#endif // !_TIMER_SYSTEM_H_