///////////////////////////////////////////////////////////////////////////////
///
///	@file  : TransformProperties.h
/// @Brief : 
///	
///	@Authors: Edwin Leow
///	@Copyright 2025, Digipen Institute of Technology
///
///////////////////////////////////////////////////////////////////////////////
#pragma once
#ifndef _TRANSFORM_PROPERTIES_H_
#define _TRANSFORM_PROPERTIES_H_

#include "PropertySystem.h"
#include "PropertyMacro.h"
#include "glm.hpp"

namespace Framework
{
    struct TransformComponent
    {
        glm::vec3 position{ 0.0f, 0.0f, 0.0f };
        glm::vec3 rotation{ 0.0f, 0.0f, 0.0f };
        glm::vec3 scale{ 1.0f, 1.0f, 1.0f };
        bool isActive = true; // Toggle active or not
    };

    inline const std::array g_TransformPropsArray =
    {
        PROP_VEC3(position, TransformComponent),
        PROP_VEC3(rotation, TransformComponent),
        PROP_VEC3(scale, TransformComponent),
        PROP_BOOL(isActive, TransformComponent),
    };

    inline const prop_list g_TransformProps{ "TransformComponent", g_TransformPropsArray };
    inline global_registration g_TransformPropsReg(&g_TransformProps);
}
#endif // !_TRANSFORM_PROPERTIES_H_