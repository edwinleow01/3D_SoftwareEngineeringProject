#pragma once
#include "pch.h"
#include "PropertySystem.h"
#include "PropertyMacro.h"
#include <glm/glm/glm.hpp>
#include <vector>

namespace Framework {

    // 1. The component struct
    struct BoxColliderComponent {
        // The local-space size of the box (like Unity's "Size")
        glm::vec3 size{ 1.0f, 1.0f, 1.0f };

        // The local-space offset of the collider (like Unity's "Center")
        glm::vec3 center{ 0.0f, 0.0f, 0.0f };
    };


    // 2. --- REGISTRATION FOR IMGUI ---
    // (This assumes you have a PROP_VEC3 macro in PropertyMacro.h)

    inline const std::vector g_BoxColliderPropsArray =
    {
        PROP_VEC3(size, BoxColliderComponent),
        PROP_VEC3(center, BoxColliderComponent)
    };

    inline const prop_list g_BoxColliderProps{ "BoxColliderComponent", g_BoxColliderPropsArray };

    inline global_registration g_BoxColliderPropsReg(&g_BoxColliderProps);
}