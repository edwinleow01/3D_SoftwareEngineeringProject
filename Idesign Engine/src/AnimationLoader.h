// AnimationLoader.h
#pragma once

#include <string>
#include "AnimationProperties.h"
namespace AnimationLoader
{
    /**
     * @brief Loads ONLY skeleton and animation data from a file
     * into an AnimationProperties struct.
     * @param path The filepath to the model (fbx, gltf).
     * @param animProps The component to populate with skeleton and animation data.
     * @return true if successful, false otherwise.
     */
    bool loadAnimation(const std::string& path, Framework::AnimationProperties& animProps);
}