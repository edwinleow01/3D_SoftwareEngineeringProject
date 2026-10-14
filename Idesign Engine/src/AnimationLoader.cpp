// AnimationLoader.cpp
#include "pch.h"
#include "AnimationLoader.h"
#include "AnimationProperties.h" // Include the full struct
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>


/**
 * @brief [HELPER] Recursively builds the bone hierarchy for the skeleton.
 */
static void ProcessNode(aiNode* node, const aiScene* scene,
    Framework::Skeleton& skeleton, int parentBoneIndex)
{
    std::string nodeName = node->mName.C_Str();
    int currentBoneIndex = -1;

    // Check if this node is a weighted bone we found in Pass 1
    auto it = skeleton.boneNameMap.find(nodeName);

    if (it != skeleton.boneNameMap.end())
    {
        // It IS a weighted bone. Update its data.
        currentBoneIndex = it->second;
    }
    else
    {
        // It is a STRUCTURAL node (like "Armature" or a Group).
        // We MUST add it to preserve the hierarchy, even if it has no weights.
        Framework::Bone structuralBone;
        structuralBone.name = nodeName;
        structuralBone.boneID = static_cast<int>(skeleton.bones.size());
        structuralBone.parentIndex = parentBoneIndex;

        // Structural nodes don't deform the mesh directly, so InverseBind is Identity
        structuralBone.inverseBindPoseMatrix = glm::mat4(1.0f);
        structuralBone.defaultLocalTransform = ConvertAssimpMatrix(node->mTransformation);

        // Register it
        skeleton.bones.push_back(structuralBone);
        skeleton.boneNameMap[nodeName] = structuralBone.boneID;
        currentBoneIndex = structuralBone.boneID;
    }

    // Update common data
    Framework::Bone& bone = skeleton.bones[currentBoneIndex];
    bone.defaultLocalTransform = ConvertAssimpMatrix(node->mTransformation);
    bone.parentIndex = parentBoneIndex; // Ensure parent is set

    // Set Root Index if this is the very first node we processed in the hierarchy
    if (parentBoneIndex == -1 && skeleton.rootBoneIndex == -1)
    {
        skeleton.rootBoneIndex = currentBoneIndex;
    }

    // Link to Parent
    if (parentBoneIndex != -1)
    {
        skeleton.bones[parentBoneIndex].childrenIndices.push_back(currentBoneIndex);
    }

    // Recurse
    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        ProcessNode(node->mChildren[i], scene, skeleton, currentBoneIndex);
    }
}

/**
 * @brief [HELPER] Loads all animation clips from the Assimp scene.
 */
static void LoadAnimations(const aiScene* scene, Framework::AnimationProperties& animProps)
{
    std::cout << "Loading " << scene->mNumAnimations << " animation(s)..." << std::endl;
    for (unsigned int i = 0; i < scene->mNumAnimations; i++)
    {
        aiAnimation* assimpAnim = scene->mAnimations[i];

        // Use std::make_shared as you did
        auto newAnim = std::make_shared<Framework::Animation>();

        newAnim->name = assimpAnim->mName.C_Str();
        if (newAnim->name.empty()) {
            newAnim->name = "Animation_" + std::to_string(i);
        }

        newAnim->ticksPerSecond = assimpAnim->mTicksPerSecond > 0 ?
            assimpAnim->mTicksPerSecond : 24.0f;
        newAnim->duration = assimpAnim->mDuration;

        for (unsigned int j = 0; j < assimpAnim->mNumChannels; j++)
        {
            aiNodeAnim* channel = assimpAnim->mChannels[j];
            std::string boneName = channel->mNodeName.C_Str();

            if (animProps.skeleton.boneNameMap.find(boneName) == animProps.skeleton.boneNameMap.end())
            {
                continue;
            }

            Framework::BoneAnimation boneAnim;
            boneAnim.boneName = boneName;

            // Load Position Keyframes
            for (unsigned int k = 0; k < channel->mNumPositionKeys; k++) {
                aiVector3D pos = channel->mPositionKeys[k].mValue;
                float time = channel->mPositionKeys[k].mTime;
                boneAnim.positions.push_back({ time, {pos.x, pos.y, pos.z} });
            }
            // Load Rotation Keyframes
            for (unsigned int k = 0; k < channel->mNumRotationKeys; k++) {
                aiQuaternion rot = channel->mRotationKeys[k].mValue;
                float time = channel->mRotationKeys[k].mTime;
                boneAnim.rotations.push_back({ time, {rot.w, rot.x, rot.y, rot.z} });
            }
            // Load Scale Keyframes
            for (unsigned int k = 0; k < channel->mNumScalingKeys; k++) {
                aiVector3D scale = channel->mScalingKeys[k].mValue;
                float time = channel->mScalingKeys[k].mTime;
                boneAnim.scales.push_back({ time, {scale.x, scale.y, scale.z} });
            }
            newAnim->boneChannels[boneName] = boneAnim;
        }

        // Store the new animation (this now works because 
        // availableAnimations should store std::shared_ptr)
        animProps.availableAnimations[newAnim->name] = newAnim;
        std::cout << "  Loaded clip: " << newAnim->name << std::endl;
    }
}

bool AnimationLoader::loadAnimation(const std::string& path, Framework::AnimationProperties& animProps)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path,
        aiProcess_Triangulate |
        aiProcess_FlipUVs |
        aiProcess_GenNormals |
        aiProcess_PopulateArmatureData
    );

    if (!scene || !scene->mRootNode || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
        std::cerr << "ASSIMP error (AnimationLoader): " << importer.GetErrorString() << std::endl;
        return false;
    }

    // Find the first mesh that has bones
    aiMesh* aimesh = nullptr;
    for (unsigned int i = 0; i < scene->mNumMeshes; i++)
    {
        aiMesh* tempMesh = scene->mMeshes[i];
        if (tempMesh->HasBones())
        {
            aimesh = tempMesh;
            break;
        }
    }

    if (aimesh == nullptr)
    {
        std::cerr << "AnimationLoader error: No mesh with bones found in file: " << path << std::endl;
        // Don't return false, it might be a valid file with no *skinned* mesh
        // but we need to check for animations.
    }

    // --- Load Skeleton Data ---
    if (aimesh && aimesh->HasBones()) // Only load skeleton if we found a skinned mesh
    {
        // PASS 1: Read aiBone data (Name, ID, Inverse Bind Matrix)
        for (unsigned int i = 0; i < aimesh->mNumBones; ++i)
        {
            aiBone* bone = aimesh->mBones[i];
            std::string boneName = bone->mName.C_Str();

            if (animProps.skeleton.boneNameMap.find(boneName) == animProps.skeleton.boneNameMap.end())
            {
                Framework::Bone newBone;
                newBone.name = boneName;
                newBone.inverseBindPoseMatrix = ConvertAssimpMatrix(bone->mOffsetMatrix);
                newBone.boneID = static_cast<int>(animProps.skeleton.bones.size());

                animProps.skeleton.bones.push_back(newBone);
                animProps.skeleton.boneNameMap[boneName] = newBone.boneID;
            }
        }


        animProps.finalBoneMatrices.resize(animProps.skeleton.bones.size(), glm::mat4(1.0f));
        animProps.skeleton.rootNodeTransform = ConvertAssimpMatrix(scene->mRootNode->mTransformation);
        animProps.globalInverseTransform = glm::inverse(animProps.skeleton.rootNodeTransform);

        // PASS 2: Read aiNode hierarchy (Parenting, Local Transforms)
        // We start processing from the *children* of the root node.
        // The root node's transform is applied separately in the AnimationSystem.
        for (unsigned int i = 0; i < scene->mRootNode->mNumChildren; i++)
        {
            std::string childName = scene->mRootNode->mChildren[i]->mName.C_Str();
            if (childName == "AnyConv" || childName == "Cauldron_Mesh" || childName == "Camera" || childName == "Light")
            {
                continue;
            }
            ProcessNode(scene->mRootNode->mChildren[i], scene, animProps.skeleton, -1);
        }
    }
 /*   std::cout << "\n--- [DEBUG] SKELETON HIERARCHY ---" << std::endl;
    std::cout << "Root Bone Index: " << animProps.skeleton.rootBoneIndex << std::endl;
    for (const auto& bone : animProps.skeleton.bones)
    {
        std::string parentName = (bone.parentIndex == -1) ? "NONE"
            : animProps.skeleton.bones[bone.parentIndex].name;

        std::cout << "  - Bone [" << bone.boneID << "] '" << bone.name
            << "' (Parent: " << bone.parentIndex << " '" << parentName << "')" << std::endl;
    }
    std::cout << "--------------------------------------\n" << std::endl;*/
    // --- Load Animation Clips ---
    if (scene->HasAnimations())
    {
        LoadAnimations(scene, animProps);
    }

    /*std::cout << "[AnimationLoader] Loaded skeleton (" << animProps.skeleton.bones.size()
        << " bones) and " << animProps.availableAnimations.size()
        << " animations from " << path << std::endl;*/

    return true;
}