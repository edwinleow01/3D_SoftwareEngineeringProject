#pragma once

#include <glm.hpp>
#include <gtc/quaternion.hpp>
#include <string>
#include <vector>
#include <map>
#include <assimp/matrix4x4.h> // Include Assimp's matrix type

// Utility function to convert Assimp's matrix to glm::mat4
// It's helpful to have this here for loading later
static inline glm::mat4 ConvertAssimpMatrix(const aiMatrix4x4& from)
{
    glm::mat4 to;
    // Assimp matrices are row-major, glm matrices are column-major
    to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
    to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
    to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
    to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
    return to;
}

namespace Framework
{
    // A single position keyframe
    struct KeyframePosition
    {
        float timeStamp = 0.0f;
        glm::vec3 position{ 0.0f };
    };

    // A single rotation keyframe (using quaternions for smooth interpolation)
    struct KeyframeRotation
    {
        float timeStamp = 0.0f;
        glm::quat rotation{ 1.0f, 0.0f, 0.0f, 0.0f }; // W, X, Y, Z (identity)
    };

    // A single scale keyframe
    struct KeyframeScale
    {
        float timeStamp = 0.0f;
        glm::vec3 scale{ 1.0f };
    };

    // Stores all keyframes for a single bone within one animation clip
    struct BoneAnimation
    {
        std::string boneName;
        std::vector<KeyframePosition> positions;
        std::vector<KeyframeRotation> rotations;
        std::vector<KeyframeScale> scales;
    };

    // An Animation Clip (e.g., "walk", "run")
    // Contains all the BoneAnimation channels for that clip
    struct Animation
    {
        std::string name;
        float duration = 0.0f;      // Duration in ticks
        float ticksPerSecond = 24.0f; // How many ticks make up one second
        std::map<std::string, BoneAnimation> boneChannels; // Map bone name to its keyframes

        // Helper to find a specific bone's animation data within this clip
        const BoneAnimation* FindChannel(const std::string& boneName) const
        {
            auto it = boneChannels.find(boneName);
            if (it == boneChannels.end())
                return nullptr; // This bone isn't animated in this clip
            return &it->second;
        }
    };

    // Represents a single bone (or joint) in the skeleton hierarchy
    struct Bone
    {
        std::string name;
        int boneID = -1;      // ID (index) in the final bone matrix array (0 to MAX_BONES-1)
        int parentIndex = -1; // Index of the parent bone in the Skeleton::bones vector (-1 for root)

        // This matrix transforms vertices from mesh space to this bone's local space in the
        // d pose
        glm::mat4 inverseBindPoseMatrix{ 1.0f };

        // The bone's default transformation relative to its parent bone
        glm::mat4 defaultLocalTransform{ 1.0f };
        std::vector<int> childrenIndices;

    };

    // Represents the entire skeleton (armature) of a mesh
    struct Skeleton
    {
        std::vector<Bone> bones;             // Flat list of all bones
        std::map<std::string, int> boneNameMap; // Map bone name to its index in the 'bones' vector
        int rootBoneIndex = -1;

        glm::mat4 rootNodeTransform{ 1.0f };
        // Helper to quickly find a bone's index by its name
        int GetBoneIndex(const std::string& name) const
        {
            auto it = boneNameMap.find(name);
            if (it != boneNameMap.end())
                return it->second;
            return -1; // Bone not found
        }
    };
} // namespace Framework