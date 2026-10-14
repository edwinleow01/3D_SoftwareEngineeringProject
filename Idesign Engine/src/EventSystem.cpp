#include "pch.h" 
#include "EventSystem.h"
#include "EventManager.h"
#include "ScriptingSystem.h"
#include <EventReceiverComponent.h> 
#include <BoxColliderComponent.h>
#include <AudioSystem.h>

extern Framework::Coordinator ecsInterface;

namespace Framework {

    void EventSystem::Initialize() {}

    void EventSystem::Update(float timeslice) {

        for (auto const& entity : mEntities) {
            if (!ecsInterface.HasComponent<EventReceiverComponent>(entity)) continue;

            // Get the component (which now holds a vector)
            auto& receiverComp = ecsInterface.GetComponent<EventReceiverComponent>(entity);

            // ITERATE THE VECTOR
            // We use a reference so we can set entry.isSubscribed = true
            for (auto& entry : receiverComp.receivers) {

                if (!entry.isSubscribed) {

                    // Capture values needed to verify validity later
                    std::string expectedChannel = entry.messageToListenFor;
                    std::string expectedLuaFunc = entry.luaFunctionToCall;

                    // [DEBUG]
                    // std::cout << "[EventSystem] (+) Entity " << entity << " subbing to '" << expectedChannel << "'" << std::endl;

                    EventManager::Instance().Subscribe(expectedChannel,
                        [entity, expectedChannel, expectedLuaFunc](void* data) -> bool {

                            // 1. CHECK: Is Entity dead?
                            if (!ecsInterface.IsEntityValid(entity)) return false;

                            // 2. CHECK: Was component removed?
                            if (!ecsInterface.HasComponent<EventReceiverComponent>(entity)) return false;

                            // 3. CHECK: Does this specific entry still exist in the vector?
                            // Because the user might have removed this specific event from the list in the Editor,
                            // but kept the component and other events.
                            bool entryFound = false;
                            auto& currentComp = ecsInterface.GetComponent<EventReceiverComponent>(entity);

                            for (const auto& currentEntry : currentComp.receivers) {
                                // We match by Channel AND Function name to ensure it's the same intent
                                if (currentEntry.messageToListenFor == expectedChannel &&
                                    currentEntry.luaFunctionToCall == expectedLuaFunc) {
                                    entryFound = true;
                                    break;
                                }
                            }

                            if (!entryFound) {
                                // The user removed this specific event entry from the array
                                return false; // Unsubscribe
                            }

                            auto scriptingSystem = ecsInterface.GetSystem<ScriptingSystem>();
                            if (scriptingSystem)
                            {
                                // Cast payload
                                auto* payload = static_cast<CollisionEventData*>(data);
                                Entity target = (payload) ? payload->targetID : -1;
                                int action = (payload) ? payload->actionType : 0;

                                // NEW TEMPLATE CALL (void version)
                                // We don't need to check if it exists first, Call handles safety.
                                scriptingSystem->Call(
                                    entity,           // The entity listening
                                    expectedLuaFunc,  // The function name to run
                                    target,           // Arg 1
                                    action            // Arg 2
                                );
                            }
                            return true; // Keep listening
                        }
                    );
                    entry.isSubscribed = true;
                }
            }
        }
    }
}