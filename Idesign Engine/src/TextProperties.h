#pragma once
#ifndef _TEXT_COMPONENT_H_
#define _TEXT_COMPONENT_H_

#include "PropertySystem.h" 
#include "PropertyMacro.h"  
#include <glm.hpp>

namespace Framework
{
    // Define the enum for clarity
    enum class TextAlignment {
        Left = 0,
        Center = 1,
        Right = 2
    };

    struct TextProperties
    {
        std::string text = "Hello World";
        float scale = 0.05f;
        glm::vec3 color = { 1.0f, 1.0f, 1.0f };
        float opacity = 1.0f;
        // [NEW] 0=Left, 1=Center, 2=Right
        int alignment = 0;
        bool isBillboard = false;

        //Jasper
        // NEW: when true, use screen-space (UI) rendering instead of world-space text
        bool screenSpace = false;
    };

    // 1. Define an array that lists each property
    inline const std::array g_TextComponentPropsArray =
    {
        PROP_STRING(text, TextProperties),
        PROP_DRAG_FLOAT(scale, TextProperties),
        PROP_COLOR3(color, TextProperties),
        PROP_DRAG_FLOAT(opacity, TextProperties),
        // [NEW] Register the integer. 
        // Note: If your property system doesn't support Enums directly, 
        // we register it as an INT here, and handle the Dropdown in the Editor logic (Step 2).
        PROP_INT(alignment, TextProperties),
        PROP_BOOL(isBillboard, TextProperties),
        PROP_BOOL(screenSpace, TextProperties)    // NEW
    };

    inline const prop_list g_TextComponentProps{ "TextProperties", g_TextComponentPropsArray };
    inline global_registration g_TextComponentReg(&g_TextComponentProps);

    // Utility: format a remaining time float into "M:SS" e.g. 299.7f -> "4:59"
    inline std::string FormatMMSS(float remainingTime)
    {
        int total = static_cast<int>(std::ceil(remainingTime));
        if (total < 0) total = 0;
        int mins = total / 60;
        int secs = total % 60;
        return std::to_string(mins) + ":" + (secs < 10 ? "0" : "") + std::to_string(secs);
    }
}
#endif // !_TEXT_COMPONENT_H_