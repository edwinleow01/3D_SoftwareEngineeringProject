// PhysicsProperties.h
#pragma once
#include "PropertySystem.h"
#include "PropertyMacro.h"
#include <glm.hpp>

namespace Framework {

    inline constexpr const char* kMotionTypeNames[] = { "Static", "Dynamic", "Kinematic" };
    inline constexpr const char* kPhysicsCategoryNames[] = { "Wall", "Floor", "Player", "Customer" };

    struct PhysicsProperties
    {
        int   motionType = 1;      // 0=Static, 1=Dynamic, 2=Kinematic
        int   category = 0;
        // New simplified UI flag:
        bool  isImmovable = false; // true = Static (0), false = Dynamic (1)
        float gravityFactor = 1.0f;   // worldGravity * factor
        float mass = 1.0f;
        float friction = 1.0f;
        float restitution = 0.0f;
        glm::vec3 linearVelocity{ 0.0f };
        glm::vec3 angularVelocity{ 0.0f };
        bool  startAwake = true;
        bool     isGrounded = false;
        glm::vec3 groundNormal{ 0.0f, 1.0f, 0.0f };

        // Tuning
        float maxSlopeAngleDeg = 50.0f;      // treat slopes as ground
        float groundCheckExtra = 0.15f;      // extra ray distance beyond collider bottom
        float stickToGroundVel = -1.0f;      // small downward bias to stay planted
    };

    // Use the same macro family your other components use.
    // (If your project uses different macro names, mirror the ones you see in e.g. Material/Text/Audio.)
    inline const std::vector g_PhysicsPropsArray = {
        PROP_INT(category,          PhysicsProperties),
        PROP_BOOL(isImmovable,      PhysicsProperties), // <-- ADD THIS
        PROP_FLOAT(gravityFactor,   PhysicsProperties),
        PROP_FLOAT(mass,            PhysicsProperties),
        PROP_FLOAT(friction,        PhysicsProperties),
        PROP_FLOAT(restitution,     PhysicsProperties),
        PROP_VEC3(linearVelocity,   PhysicsProperties),
        PROP_VEC3(angularVelocity,  PhysicsProperties),
        PROP_BOOL(startAwake,       PhysicsProperties),
    };

    inline const prop_list g_PhysicsProps{ "PhysicsProperties", g_PhysicsPropsArray };
    inline global_registration g_PhysicsPropsReg(&g_PhysicsProps);

} // namespace Framework
