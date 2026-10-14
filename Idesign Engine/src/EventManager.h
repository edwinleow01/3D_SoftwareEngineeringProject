#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace Framework {

    class EventManager {
    public:
        using EventCallback = std::function<bool(void*)>;

        // RENAMED FROM Get() TO Instance() TO MATCH YOUR CODE
        static EventManager& Instance() {
            static EventManager instance;
            return instance;
        }

        void Subscribe(const std::string& eventName, EventCallback callback);
        void Fire(const std::string& eventName, void* payload = nullptr);
        void Reset();

    private:
        EventManager() = default;
        std::unordered_map<std::string, std::vector<EventCallback>> mListeners;
    };

}