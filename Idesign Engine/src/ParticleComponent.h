#pragma once
#ifndef PARTICLE_COMPONENT_H
#define PARTICLE_COMPONENT_H

#include <vector>
#include <glm.hpp>

#include "AssetHandler.h"
#include "PropertySystem.h"
#include "PropertyMacro.h"

namespace Framework
{
    // Runtime particle (NOT reflected/serialized)
    struct Particle
    {
        glm::vec3 position{ 0.0f };
        glm::vec3 velocity{ 0.0f };
        float age = 0.0f;
        float lifetime = 1.0f;
        float size = 1.0f;
    };

    // Emitter component (reflected/serialized fields first)
    struct ParticleEmitterComponent
    {
        int style = 0;

        // --- Requested controls ---
        bool active = true;

        // Loop vs duration
        bool  loop = true;
        float duration = 1.0f;     // only used if loop == false

        // Spray controls
        glm::vec3 direction{ 0.0f, 1.0f, 0.0f }; // mean direction (normalized at runtime)
        float spreadAngleDeg = 20.0f;            // cone half-angle
        float range = 3.0f;                      // approx travel distance
        float particlesPerSecond = 60.0f;        // number of particles in the spray (rate)
        float particleSpeed = 5.0f;              // particle speed
        float particleSize = 0.2f;               // billboard scale

        // Local/world
        bool localSpace = true;
        glm::vec3 emitterOffset{ 0.0f, 0.0f, 0.0f }; // local/world spawn offset from entity center

        // Spawn shape
        int emissionShape = 0; // 0 = Point, 1 = Box, 2 = Sphere
        glm::vec3 emissionBoxHalfExtents{ 0.0f, 0.0f, 0.0f };
        float emissionRadius = 0.0f;

        // Billboard PNG texture selection
        AssetHandle textureGuid;   // references TextureProperties in AssetManager
        bool pixelFilter = true;   // force nearest filtering for pixel look

        // Safety cap
        int maxParticles = 2000;

        // --- Runtime-only ---
        unsigned int textureID = 0;  // cached GL id resolved from textureGuid
        float timeSinceStart = 0.0f;
        float emitAccumulator = 0.0f;
        std::vector<Particle> particles;
    };

    // Reflection (Inspector + Scene serialize)
    inline const std::vector g_ParticleEmitterPropsArray =
    {
        PROP_BOOL(active,                 ParticleEmitterComponent),

        PROP_BOOL(loop,                   ParticleEmitterComponent),
        PROP_FLOAT(duration,              ParticleEmitterComponent),

        PROP_VEC3(direction,              ParticleEmitterComponent),
        PROP_FLOAT(spreadAngleDeg,        ParticleEmitterComponent),
        PROP_FLOAT(range,                 ParticleEmitterComponent),

        PROP_FLOAT(particlesPerSecond,    ParticleEmitterComponent),
        PROP_FLOAT(particleSpeed,         ParticleEmitterComponent),
        PROP_FLOAT(particleSize,          ParticleEmitterComponent),

        PROP_BOOL(localSpace,             ParticleEmitterComponent),
        PROP_VEC3(emitterOffset,           ParticleEmitterComponent),

        PROP_INT(emissionShape,            ParticleEmitterComponent),
        PROP_VEC3(emissionBoxHalfExtents,  ParticleEmitterComponent),
        PROP_FLOAT(emissionRadius,         ParticleEmitterComponent),

        // Serialize the chosen texture GUID, but hide it in generic inspector
        PROP_GUID_FLAGS(textureGuid,      ParticleEmitterComponent, Framework::prop::HIDDEN),
        PROP_BOOL(pixelFilter,            ParticleEmitterComponent),

        PROP_INT(maxParticles,            ParticleEmitterComponent),
    };

    inline const prop_list g_ParticleEmitterProps{ "ParticleEmitterComponent", g_ParticleEmitterPropsArray };
    inline global_registration g_ParticleEmitterReg(&g_ParticleEmitterProps);
}

#endif
