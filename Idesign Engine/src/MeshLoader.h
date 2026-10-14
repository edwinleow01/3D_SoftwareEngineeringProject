// MeshLoader.h
#pragma once
#include <string>
#include "MeshProperties.h"
#include "AnimationProperties.h"

using namespace Framework;


class MeshLoader {


public:
    static void loadMesh(const std::string& path, MeshProperties& mesh);
    static void GenerateDebugMesh(MeshProperties& mesh);
    /**
    * @brief Loads an ANIMATED mesh (with skeleton and bones).
    */
    static void loadAnimatedMesh(const std::string& path,
        Framework::AnimationProperties& animProps,
        Framework::MeshProperties& mesh);
};
