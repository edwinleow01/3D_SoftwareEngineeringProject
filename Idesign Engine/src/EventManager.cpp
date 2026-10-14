// EventManager.cpp
#include "pch.h"
#include "EventManager.h"
#include <iostream>

namespace Framework {

    void EventManager::Subscribe(const std::string& eventName, EventCallback callback) {
        mListeners[eventName].push_back(callback);
    }

    void EventManager::Fire(const std::string& eventName, void* payload) {
        if (mListeners.find(eventName) == mListeners.end()) return;

        auto& listeners = mListeners[eventName];

        // [CRITICAL FIX] Iterate using an iterator so we can erase efficiently
        for (auto it = listeners.begin(); it != listeners.end(); ) {
            
            // Call the function. It now returns a bool.
            bool keepSubscribed = (*it)(payload);

            if (!keepSubscribed) {
                // Listener said "I'm dead", so we erase it
                it = listeners.erase(it);
            }
            else {
                // Listener is good, move to next
                ++it;
            }
        }
    }

    void EventManager::Reset() {
        mListeners.clear();
    }
}