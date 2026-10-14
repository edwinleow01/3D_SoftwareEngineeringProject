
#pragma once
#ifndef _MATERIAL_PROPERTIES_H_
#define _MATERIAL_PROPERTIES_H_
#include "PropertySystem.h"
#include "PropertyMacro.h"
#include <glm.hpp>  // Needed for PROP_COLOR3
namespace Framework
{
    struct MaterialMetadata
    {
        AssetHandle guid;
        std::string name;
        std::string path;
    };

    inline const std::vector g_MaterialMetaPropsArray = 
    {
        PROP_GUID_HIDDEN(guid,   MaterialMetadata),
        PROP_STRING_HIDDEN(name, MaterialMetadata),
        PROP_STRING_HIDDEN(path, MaterialMetadata)
    };

    inline const prop_list g_MaterialMetaProps{ "MaterialMetadata", g_MaterialMetaPropsArray };
    inline global_registration g_MaterialMetaPropsReg(&g_MaterialMetaProps);

//------------------------------------------------------------------------------------------------------------------//
    
    struct MaterialProperties
    {
        AssetHandle guid;

        // --- ADDED MATERIAL PROPERTIES ---
        glm::vec3 ambient{ 0.1f, 0.1f, 0.1f };
        glm::vec3 albedo{ 1.0f, 1.0f, 1.0f };   // This will be the tint for your texture
        glm::vec3 specular{ 1.0f, 1.0f, 1.0f }; // Bright white specular for metals/plastics
        float shininess = 32.0f;                // Common default shininess
        float opacity = 1.0f;
    };

    inline const std::vector g_MaterialPropsArray =
    {
        PROP_GUID_HIDDEN(guid,      MaterialProperties),
        PROP_COLOR3(ambient,        MaterialProperties),
        PROP_COLOR3(albedo,         MaterialProperties),
        PROP_COLOR3(specular,       MaterialProperties),
        PROP_DRAG_FLOAT(shininess,  MaterialProperties),
        PROP_DRAG_FLOAT(opacity,    MaterialProperties)
    };

    inline const prop_list g_MaterialProps{ "MaterialProperties", g_MaterialPropsArray };
    inline global_registration g_MaterialPropsReg(&g_MaterialProps);
}
#endif // !_MATERIAL_PROPERTIES_H_