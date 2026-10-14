///////////////////////////////////////////////////////////////////////////////
///
/// @file   : NavMeshComponent.h
/// @brief  : Runtime navmesh generation settings and transient runtime data
/// 
/// @authors: Joshua Sim 
/// @copyright 2025, Digipen Institute of Technology
///
///////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef _NAVMESH_COMPONENT_H_
#define _NAVMESH_COMPONENT_H_

#include "PropertySystem.h"
#include "PropertyMacro.h"
#include <vector>
#include <string>
#include <glm.hpp>

namespace Framework
{
    struct NavMeshComponent
    {
        std::string guid;  // Reference to MeshProperties
        bool isNavMeshSource = true; //default to true
        bool runtimeGenerated = true;

        // Recast config settings
        float agentRadius = 0.5f;
        float agentHeight = 2.0f;
        float cellSize = 0.3f;
        float cellHeight = 0.2f;
        float maxSlope = 45.0f;

        // Non-serialized runtime buffers (used for navmesh generation or pathfinding)
        std::vector<glm::vec3> positions;     // Reconstructed from MeshProperties on demand
        std::vector<int> indices;             // Triangle indices

        // Optional: store dtNavMesh*, dtNavMeshQuery* pointers separately if used
    };

    // Serialized properties (only these go into JSON)
    // Serialized properties (only these go into JSON)
    inline std::array g_NavMeshPropsArray =
    {
        PROP_STRING_HIDDEN(guid, NavMeshComponent),
        PROP_BOOL(isNavMeshSource, NavMeshComponent),
        PROP_BOOL(runtimeGenerated, NavMeshComponent),
        PROP_DRAG_FLOAT(agentRadius, NavMeshComponent),
        PROP_DRAG_FLOAT(agentHeight, NavMeshComponent),
        PROP_DRAG_FLOAT(cellSize, NavMeshComponent),
        PROP_DRAG_FLOAT(cellHeight, NavMeshComponent),
        PROP_DRAG_FLOAT(maxSlope, NavMeshComponent),
    };

    inline constexpr prop_list g_NavMeshProps{ "NavMeshComponent", g_NavMeshPropsArray };
    inline global_registration g_NavMeshPropsReg(&g_NavMeshProps);
}

#endif // !_NAVMESH_COMPONENT_H_
