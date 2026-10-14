///////////////////////////////////////////////////////////////////////////////
///
/// @file   : NavMeshAgentComponent.h
/// @brief  : Component for entities that navigate using the navmesh
/// 
/// @authors: Joshua Sim 
/// @copyright 2025, Digipen Institute of Technology
///
///////////////////////////////////////////////////////////////////////////////
#pragma once
#ifndef _NAVMESH_AGENT_COMPONENT_H_
#define _NAVMESH_AGENT_COMPONENT_H_

#include "PropertySystem.h"
#include "PropertyMacro.h"
#include <vector>
#include <glm.hpp>

namespace Framework
{
    struct NavMeshAgentComponent
    {
        // Agent physical properties (should match navmesh build parameters)
        float agentHeight = 2.0f;       // Height of the agent
        float agentRadius = 0.6f;       // Radius of the agent

        // Movement properties
        float moveSpeed = 3.5f;         // Movement speed in units/second
        float angularSpeed = 120.0f;    // Rotation speed in degrees/second
        float acceleration = 8.0f;      // How quickly agent reaches max speed
        float rotationOffset = 0.0f;  // adjust rotation angle offset for different models
        float stoppingDistance = 0.1f;  // Distance to waypoint before moving to next

        // Pathfinding behavior
        bool autoRepath = false;        // Automatically recalculate path if target moves
        bool stopOnTargetReached = true; // Stop moving when destination reached

        // Target destination
        glm::vec3 targetPosition = glm::vec3(0.0f);


        // --- Non-serialized runtime data ---
        std::vector<glm::vec3> currentPath;  // Current path waypoints
        int currentWaypointIndex = 0;        // Index of current waypoint
        bool hasPath = false;                // Whether agent has a valid path
        bool isMoving = false;               // Whether agent is currently moving
        float currentSpeed = 0.0f;           // Current movement speed (for acceleration)
    };

    // Serialized properties (saved to JSON)
    inline std::array g_NavMeshAgentPropsArray =
    {
        PROP_FLOAT(agentHeight, NavMeshAgentComponent),
        PROP_FLOAT(agentRadius, NavMeshAgentComponent),
        PROP_FLOAT(moveSpeed, NavMeshAgentComponent),
        PROP_FLOAT(angularSpeed, NavMeshAgentComponent),
        PROP_FLOAT(rotationOffset, NavMeshAgentComponent),  
        PROP_FLOAT(acceleration, NavMeshAgentComponent),
        PROP_FLOAT(stoppingDistance, NavMeshAgentComponent),
        PROP_BOOL(autoRepath, NavMeshAgentComponent),
        PROP_BOOL(stopOnTargetReached, NavMeshAgentComponent),
        PROP_VEC3(targetPosition, NavMeshAgentComponent),
    };

    inline constexpr prop_list g_NavMeshAgentProps{ "NavMeshAgentComponent", g_NavMeshAgentPropsArray };
    inline global_registration g_NavMeshAgentPropsReg(&g_NavMeshAgentProps);
}

#endif // !_NAVMESH_AGENT_COMPONENT_H_