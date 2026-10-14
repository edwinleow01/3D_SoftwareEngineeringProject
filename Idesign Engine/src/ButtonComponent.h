#pragma once
#ifndef _BUTTON_COMPONENT_H_
#define _BUTTON_COMPONENT_H_

#include "PropertySystem.h" 
#include "PropertyMacro.h" 
#include <glm.hpp>
#include <functional>
#include <iostream>
#include "AudioSystem.h"

namespace Framework
{
    struct ButtonComponent
    {
        glm::vec2 position;
        glm::vec2 size;
        bool isHovered = false;
        std::function<void()> onClick;
        bool isFrontButton = false;
        bool interactable = true;
        AssetHandle hoverTextureGuid = ""; // The GUID picked in ImGui
        unsigned int hoverTextureID = 0;
        unsigned int originalTextureID = 0;
        AssetHandle originalTextureGuid = ""; // ADD THIS to remember the original string

        void TriggerClick(Entity e) const {
            if (!interactable) return;
            if (onClick) {
                g_AudioSystem->PlayAudio("event:/ButtonClick");
                onClick();
            }
        }
    };

    // Register the new property for serialization/ImGui
    inline const std::vector g_ButtonComponentPropsArray = {
        PROP_VEC2(position, ButtonComponent),
        PROP_VEC2(size, ButtonComponent),
        PROP_BOOL(isHovered, ButtonComponent),
        PROP_BOOL(isFrontButton, ButtonComponent),
        PROP_GUID_HIDDEN(hoverTextureGuid, ButtonComponent),
    };

    inline const prop_list g_ButtonComponentProps{ "ButtonComponent", g_ButtonComponentPropsArray };
    inline global_registration g_ButtonComponentReg(&g_ButtonComponentProps);
}
#endif // !_BUTTON_COMPONENT_H_