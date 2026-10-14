#pragma once
#ifndef _TIMER_COMPONENT_H_
#define _TIMER_COMPONENT_H_

#include "PropertySystem.h"
#include "PropertyMacro.h"
#include <functional>
#include <iostream>

namespace Framework
{
    struct TimerComponent
    {
        float duration = 10.0f;             // Total duration of the timer (seconds)
        float remainingTime = 10.0f;        // Time left (seconds)
        bool  isRunning = false;            // Is the timer actively counting down?
        bool  isFinished = false;           // Has the timer reached 0?
        bool  loop = false;                 // Should the timer restart after finishing?
        std::function<void()> onComplete;   // Callback fired when timer reaches 0

        // Start / restart the timer
        void Start()
        {
            remainingTime = duration;
            isRunning = true;
            isFinished = false;
        }

        void Start(float newDuration)
        {
            duration = newDuration;
            remainingTime = newDuration;
            isRunning = true;
            isFinished = false;
        }

        void Pause() { isRunning = false; }
        void Resume() { if (!isFinished) isRunning = true; }

        void Reset()
        {
            remainingTime = duration;
            isRunning = false;
            isFinished = false;
        }

        // Call this every frame from your timer system
        void Update(float dt)
        {
            if (!isRunning || isFinished) return;

            remainingTime -= dt;

            if (remainingTime <= 0.0f)
            {
                remainingTime = 0.0f;
                isFinished = true;
                isRunning = false;

                if (onComplete)
                {
                    onComplete();
                }
                else
                {
                    std::cout << "[Timer] Timer finished with no onComplete callback.\n";
                }

                if (loop)
                {
                    Start(); // restart automatically
                }
            }
        }

        // Normalised progress 0.0 (empty) -> 1.0 (full), useful for progress bars
        float GetProgress() const
        {
            if (duration <= 0.0f) return 0.0f;
            return remainingTime / duration;
        }

        // Elapsed time since the timer started
        float GetElapsed() const
        {
            return duration - remainingTime;
        }
    };

    // Property registration for TimerComponent
    // Note: onComplete is a function and is not serializable, same as onClick in ButtonComponent
    inline const std::vector g_TimerComponentPropsArray =
    {
        PROP_FLOAT(duration,      TimerComponent),
        PROP_FLOAT(remainingTime, TimerComponent),
        PROP_BOOL(isRunning,      TimerComponent),
        PROP_BOOL(isFinished,     TimerComponent),
        PROP_BOOL(loop,           TimerComponent),
    };

    inline const prop_list g_TimerComponentProps{ "TimerComponent", g_TimerComponentPropsArray };
    inline global_registration g_TimerComponentReg(&g_TimerComponentProps);
}
#endif // !_TIMER_COMPONENT_H_