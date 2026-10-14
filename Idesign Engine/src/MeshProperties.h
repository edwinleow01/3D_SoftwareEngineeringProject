///////////////////////////////////////////////////////////////////////////////
///
///	@file  : MeshProperties.h
/// @Brief : 
///	
///	@Authors: Edwin Leow
///	@Copyright 2025, Digipen Institute of Technology
///
///////////////////////////////////////////////////////////////////////////////
#pragma once
#ifndef _MESH_PROPERTIES_H_
#define _MESH_PROPERTIES_H_

#include "PropertySystem.h"
#include "PropertyMacro.h"
#include <glm.hpp>
namespace Framework
{
    enum class DebugShapeType
    {
        None,
        Box,
        Sphere,
        Triangle
    };

    struct MeshProperties
    {
        AssetHandle guid;
        std::string name;
        std::string path; // asset path to load and fill in mesh component vars, alloted default
        unsigned int vao = 0, vbo = 0, ebo = 0;
        unsigned int indexCount = 0;
        bool isDirty = false;

        //debug rendering 
        std::vector<glm::vec3> positions; 
        std::vector<int> indices;         
        DebugShapeType debugShape = DebugShapeType::None;
        glm::vec3 debugcolor{ 1.f,1.f,0.0f };

        AssetHandle materialGuid;   // Reference to a material asset
    };

    inline const std::vector g_MeshPropsArray =
    {
        PROP_GUID_HIDDEN(guid, MeshProperties),
        PROP_STRING_HIDDEN(name, MeshProperties),
        PROP_STRING_HIDDEN(path, MeshProperties),
        PROP_UINT_READONLY(vao, MeshProperties),
        PROP_UINT_READONLY(vbo, MeshProperties),
        PROP_UINT_READONLY(ebo, MeshProperties),
        PROP_UINT_READONLY(indexCount, MeshProperties),
    };

    inline const prop_list g_MeshProps{ "MeshProperties", g_MeshPropsArray };
    inline global_registration g_MeshPropsReg(&g_MeshProps);
}
#endif // !_MESH_PROPERTIES_H_