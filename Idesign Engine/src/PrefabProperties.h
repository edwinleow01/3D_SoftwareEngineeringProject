///////////////////////////////////////////////////////////////////////////////
///
/// @file  : PrefabProperties.h
/// @Brief : Properties for prefab assets
///	
/// @Authors: Edwin Leow
/// @Copyright 2025, Digipen Institute of Technology
///
///////////////////////////////////////////////////////////////////////////////
#pragma once
#ifndef _PREFAB_PROPERTIES_H_
#define _PREFAB_PROPERTIES_H_
#include "PropertySystem.h"
#include "PropertyMacro.h"
#include <glm.hpp>

namespace Framework
{
    struct PrefabProperties
    {
        AssetHandle guid;           // Unique prefab GUID
        std::string name;           // "Orc", "Tree", etc.
        std::string path;           // "Assets/Prefabs/Orc.prefab"
        bool isScenePrefab = false; // true = multi-entity, false = single entity
    };

    inline const std::vector g_PrefabPropsArray =  // <-- Added '=' here!
    {
        PROP_GUID_READONLY(guid, PrefabProperties),  // <-- Use PROP_GUID not PROP_ASSETHANDLE
        PROP_STRING(name, PrefabProperties),
        PROP_STRING_READONLY(path, PrefabProperties),
        PROP_BOOL_READONLY(isScenePrefab, PrefabProperties)
    };

    inline const prop_list g_PrefabProps{ "PrefabProperties", g_PrefabPropsArray };
    inline global_registration g_PrefabPropsReg(&g_PrefabProps);
}
#endif // !_PREFAB_PROPERTIES_H_