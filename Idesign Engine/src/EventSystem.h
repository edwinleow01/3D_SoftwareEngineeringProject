#pragma once
#include "Coordinator.h"
#include <string>

namespace Framework {

    // Inherit from ISystem (the interface you shared)
    class EventSystem : public ISystem {
    public:
        // 1. FIX: Rename Init -> Initialize to match the base class
        void Initialize() override;

        // 2. FIX: Implement Update to satisfy pure virtual = 0
        void Update(float timeslice) override;

        // 3. FIX: Implement GetName to satisfy pure virtual = 0
        std::string GetName() override { return "EventSystem"; }
    };
}