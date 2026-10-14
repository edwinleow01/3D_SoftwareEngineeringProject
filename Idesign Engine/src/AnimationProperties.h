// AnimationProperties.h
#pragma once
#ifndef _ANIMATION_PROPERTIES_H_
#define _ANIMATION_PROPERTIES_H_

#include "PropertySystem.h"
#include "PropertyMacro.h"
#include "animation.h"
#include <glew.h>
#include <glm.hpp>
#include <vector>
#include <map>
#include <string>
#include <memory>

namespace Framework
{
    /**
     * @brief ECS Component for animated entities.
     * Contains asset links, configurable fields, and runtime data.
     */
    struct AnimationProperties
    {
        // --- ASSET FIELDS ---
        AssetHandle guid;
        std::string name;
        std::string path; // Path to the .fbx, .gltf, etc.
        int updateMode = 0;
        /**
         * @brief The name of the animation to play by default.
         * (e.g., "Armature|ArmatureAction")
         */
        std::string defaultAnimationName = "";

        float animationSpeed = 1.0f;
        bool bLoops = true;
        bool bPlayOnStart = true;


        float currentTime = 0.0f;
        std::shared_ptr<Animation> currentAnimation = nullptr;
        unsigned int vbo_skinning = 0; // VBO for bone IDs/weights

        std::vector<glm::mat4> finalBoneMatrices;
        std::map<std::string, std::shared_ptr<Animation>> availableAnimations;
        Skeleton skeleton;

        glm::mat4 globalInverseTransform{ 1.0f };
        // --- FUNCTIONS ---
        void PlayAnimation(const std::string& name)
        {
            auto it = availableAnimations.find(name);
            if (it != availableAnimations.end())
            {
                if (currentAnimation != it->second)
                {
                    currentAnimation = it->second;
                    currentTime = 0.0f;
                }
            }
        }

        void StartDefaultAnimation()
        {
            if (bPlayOnStart && !defaultAnimationName.empty())
            {
                PlayAnimation(defaultAnimationName);
            }
            else if (bPlayOnStart && !availableAnimations.empty())
            {
                // Fallback: just play the first available animation
                PlayAnimation(availableAnimations.begin()->first);
            }
        }
    };

    //
    // --- PROPERTY REGISTRATION ---
    //
    inline const std::vector g_AnimationPropertiesPropsArray =
    {
        PROP_GUID_HIDDEN(guid, AnimationProperties),
        PROP_STRING_HIDDEN(name, AnimationProperties),
        PROP_STRING_HIDDEN(path, AnimationProperties),
        PROP_STRING_HIDDEN(defaultAnimationName, AnimationProperties),
        PROP_DRAG_FLOAT_HIDDEN(animationSpeed, AnimationProperties),
        PROP_BOOL_HIDDEN(bLoops, AnimationProperties),
        PROP_BOOL_HIDDEN(bPlayOnStart, AnimationProperties),
        PROP_DRAG_FLOAT_HIDDEN(currentTime, AnimationProperties),
        PROP_UINT_HIDDEN(vbo_skinning, AnimationProperties)
    };

    
    inline const prop_list g_AnimationPropertiesProps{ "AnimationProperties", g_AnimationPropertiesPropsArray };
    inline global_registration g_AnimationPropertiesPropsReg(&g_AnimationPropertiesProps);

} // namespace Framework

#endif // !_ANIMATION_PROPERTIES_H_