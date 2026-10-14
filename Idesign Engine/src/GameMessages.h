#pragma once
#include "Coordinator.h"
#include "MessageSystem.h" // <-- MUST include this

namespace Framework {

    /**
     * @brief Fired when one entity 'interacts' with another.
     */

     // THIS IS THE FIX:
     // It now inherits from 'TypedMessage' which provides all the
     // GetTypeIndex() and GetStaticTypeIndex() functions.
    struct PlayerInteractedWithItem : public Framework::TypedMessage<PlayerInteractedWithItem> {

        Entity interactor; // The entity that did the interacting (e.g., the Player)
        Entity interactee; // The entity that was interacted with (e.g., the Herb)

        // Add a constructor (optional but good practice)
        PlayerInteractedWithItem(Entity i, Entity e) : interactor(i), interactee(e) {}
    };

    struct EntityClickedMessage : public Framework::TypedMessage<EntityClickedMessage> {

        Entity clicker; // The entity that did the clicking (e.g., the Player)
        Entity target;  // The entity that was clicked (e.g., the Button)

        EntityClickedMessage(Entity c, Entity t) : clicker(c), target(t) {}
    };

}