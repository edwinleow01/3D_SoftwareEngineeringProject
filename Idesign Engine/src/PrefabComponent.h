///////////////////////////////////////////////////////////////////////////////
///
/// @file  : PrefabComponent.h
/// @brief : Component that marks an entity as a prefab instance
///
/// @Authors: Edwin Leow
/// @Copyright 2025, Digipen Institute of Technology
///
///////////////////////////////////////////////////////////////////////////////
#pragma once
#ifndef _PREFAB_COMPONENT_H_
#define _PREFAB_COMPONENT_H_

#include "PropertySystem.h"
#include "PropertyMacro.h"
#include "AssetHandler.h"

namespace Framework
{
    struct PrefabComponent
    {
        AssetHandle prefabGUID;  // References PrefabProperties in AssetManager

        // std::vector<std::string> addedComponents;      // Component types added to instance
        // std::vector<std::string> deletedComponents;    // Component types removed from prefab
        
        // For delta-on change
        // 
        // std::vector<PropertyOverride> overriddenProperties;
    };

    inline const std::array g_PrefabComponentPropsArray =
    {
        PROP_STRING(prefabGUID.id, ::PrefabComponent),  // Access the string directly

        // Delta properties 
        // PROP_STRING_ARRAY(addedComponents, PrefabComponent),
        // PROP_STRING_ARRAY(deletedComponents, PrefabComponent),
    };

    inline const prop_list g_PrefabComponentProps{ "PrefabComponent", g_PrefabComponentPropsArray };
    inline global_registration g_PrefabComponentPropsReg(&g_PrefabComponentProps);
}

#endif // !_PREFAB_COMPONENT_H_