
#pragma once
#ifndef _SCENE_PROPERTIES_H_
#define _SCENE_PROPERTIES_H_
#include "PropertySystem.h"
#include "PropertyMacro.h"
namespace Framework
{
    struct SceneProperties
    {
        AssetHandle guid;
        std::string name;
        std::string path;
    };

    inline const std::vector g_ScenePropsArray =
    {
        PROP_GUID_READONLY(guid, SceneProperties),
        PROP_STRING(name, SceneProperties),
        PROP_STRING(path, SceneProperties)
    };

    inline const prop_list g_SceneProps{ "SceneProperties", g_ScenePropsArray };
    inline global_registration g_ScenePropertiesReg(&g_SceneProps);
}
#endif // !_SCENE_PROPERTIES_H_