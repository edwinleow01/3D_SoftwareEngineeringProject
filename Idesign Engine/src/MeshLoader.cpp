#include "pch.h"
#include "MeshLoader.h"
#include <glew.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>
#include <vector>
#include "EnginePath.h" // <-- Add this include
#include <filesystem>    // <-- Make sure this is included for path joining
#include <glm.hpp>
#include <assimp/matrix4x4.h>
#include "AnimationProperties.h" // Includes Skeleton.h
#include "AnimationLoader.h"
#include "Mesh.h" // Make sure your Mesh class is included here

#define MAX_BONES_PER_VERTEX 4

// --- Helper Structs ---
struct Vertex {
    float pos[3];
    float normal[3];
    float tex[2];
};


struct AnimatedVertex {
    float pos[3];
    float normal[3];
    float tex[2];
    glm::ivec4 boneIDs{ -1, -1, -1, -1 };
    glm::vec4 weights{ 0.0f, 0.0f, 0.0f, 0.0f };
};

struct VertexSkinning
{
    glm::ivec4 boneIDs{ -1, -1, -1, -1 };
    glm::vec4 weights{ 0.0f, 0.0f, 0.0f, 0.0f };

    void AddBoneData(int boneID, float weight) {
        for (int i = 0; i < MAX_BONES_PER_VERTEX; ++i) {
            if (boneIDs[i] == -1) {
                boneIDs[i] = boneID;
                weights[i] = weight;
                return;
            }
        }
    }
};



void MeshLoader::loadMesh(const std::string& path, Framework::MeshProperties& mesh) {
    Assimp::Importer importer;

    // --- 1. Construct the full, absolute path ---
    std::filesystem::path root = GetEngineRootPath();
    std::filesystem::path absolutePath = root / path;
    // ---------------------------------------------

    const aiScene* scene = importer.ReadFile(
        absolutePath.string(), // <-- 2. Use the absolute path here
        aiProcess_Triangulate | aiProcess_GenNormals
    );

    if (!scene || !scene->mRootNode || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
        // 3. (Optional) Improve error message to show the path it tried
        // std::cerr << "ASSIMP error loading " << absolutePath.string() << ": " << importer.GetErrorString() << std::endl;
        return;
    }
    if (scene->mNumMeshes == 0) {
        std::cerr << "No meshes found in file: " << absolutePath.string() << std::endl;
        return;
    }

    aiMesh* aimesh = scene->mMeshes[0];

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    // (Vertex data loading...)
    for (unsigned int i = 0; i < aimesh->mNumVertices; i++) {
        Vertex v{};
        v.pos[0] = aimesh->mVertices[i].x;
        v.pos[1] = aimesh->mVertices[i].y;
        v.pos[2] = aimesh->mVertices[i].z;
        if (aimesh->HasNormals()) {
            v.normal[0] = aimesh->mNormals[i].x;
            v.normal[1] = aimesh->mNormals[i].y;
            v.normal[2] = aimesh->mNormals[i].z;
        }
        if (aimesh->HasTextureCoords(0)) {
            v.tex[0] = aimesh->mTextureCoords[0][i].x;
            v.tex[1] = aimesh->mTextureCoords[0][i].y;
        }
        else {
            v.tex[0] = v.tex[1] = 0.0f;
        }
        vertices.push_back(v);
    }

    // (Index data loading...)
    for (unsigned int i = 0; i < aimesh->mNumFaces; i++) {
        aiFace face = aimesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    // (OpenGL Buffer loading... this part is fine)
    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glGenBuffers(1, &mesh.ebo);
    glBindVertexArray(mesh.vao);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(6 * sizeof(float)));

    glBindVertexArray(0);
    mesh.indexCount = static_cast<unsigned int>(indices.size());

    // Store geometry for navmesh or other CPU use
    mesh.positions.clear();
    mesh.indices.clear();
    for (const auto& v : vertices) {
        mesh.positions.emplace_back(v.pos[0], v.pos[1], v.pos[2]);
    }
    mesh.indices.assign(indices.begin(), indices.end());

    //std::cout << mesh.guid << " " << mesh.path << " was loaded into system" << std::endl;
}

// --- ANIMATED MESH LOADER---
void MeshLoader::loadAnimatedMesh(const std::string& path,
    Framework::AnimationProperties& animProps,
    Framework::MeshProperties& mesh)
{
    // Check if skeleton is ready. If not, this whole process will fail.
    if (animProps.skeleton.bones.empty()) {
        // Fallback to static loader to prevent crash
        loadMesh(path, mesh);
        return;
    }

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path,
        aiProcess_Triangulate |
        aiProcess_GenNormals |
        aiProcess_PopulateArmatureData 
    );

    if (!scene || !scene->mRootNode || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
        std::cerr << "ASSIMP error: " << importer.GetErrorString() << std::endl;
        return;
    }

    aiMesh* aimesh = nullptr;
    for (unsigned int i = 0; i < scene->mNumMeshes; i++)
    {
        if (scene->mMeshes[i]->HasBones()) {
            aimesh = scene->mMeshes[i];
        /*    std::cout << "[MeshLoader] Found skinned mesh '"
                << aimesh->mName.C_Str() << "' at index " << i << std::endl;*/
            break;
        }
    }

    if (aimesh == nullptr) {
        std::cerr << "ASSIMP error: No mesh with bones found in file: " << path << std::endl;
        loadMesh(path, mesh); // Fallback to static
        return;
    }

    std::vector<AnimatedVertex> vertices(aimesh->mNumVertices);
    std::vector<unsigned int> indices;
    std::vector<VertexSkinning> skinning_data(aimesh->mNumVertices);

    // --- Load Vertices (Pos, Normal, UV) ---
    for (unsigned int i = 0; i < aimesh->mNumVertices; i++) {
        vertices[i].pos[0] = aimesh->mVertices[i].x;
        vertices[i].pos[1] = aimesh->mVertices[i].y;
        vertices[i].pos[2] = aimesh->mVertices[i].z;
        if (aimesh->HasNormals()) {
            vertices[i].normal[0] = aimesh->mNormals[i].x;
            vertices[i].normal[1] = aimesh->mNormals[i].y;
            vertices[i].normal[2] = aimesh->mNormals[i].z;
        }
        if (aimesh->HasTextureCoords(0)) {
            vertices[i].tex[0] = aimesh->mTextureCoords[0][i].x;
            vertices[i].tex[1] = aimesh->mTextureCoords[0][i].y;
        }
    }

    // --- Load Indices ---
    for (unsigned int i = 0; i < aimesh->mNumFaces; i++) {
        aiFace face = aimesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }


    for (unsigned int i = 0; i < aimesh->mNumBones; ++i)
    {
        aiBone* bone = aimesh->mBones[i];
        std::string boneName = bone->mName.C_Str();

        // Get the boneID from the skeleton 
        int boneID = animProps.skeleton.GetBoneIndex(boneName);

        if (boneID == -1) {
            std::cout << "Warning [MeshLoader]: Bone '" << boneName << "' (from mesh) not found in final skeleton. Skipping." << std::endl;
            continue; // Skip this bone; its weights will be 0
        }

        // Assign weights to vertices 
        for (unsigned int j = 0; j < bone->mNumWeights; ++j)
        {
            const aiVertexWeight& weight = bone->mWeights[j];
            skinning_data[weight.mVertexId].AddBoneData(boneID, weight.mWeight);
        }
    }
    std::cout << "[MeshLoader] Bone mapping complete." << std::endl;


    // Copy skinning data into the main vertex buffer
    for (unsigned int i = 0; i < aimesh->mNumVertices; i++) {
        vertices[i].boneIDs = skinning_data[i].boneIDs;
        vertices[i].weights = skinning_data[i].weights;
        float totalWeight = vertices[i].weights.x + vertices[i].weights.y + vertices[i].weights.z + vertices[i].weights.w;
        if (totalWeight > 0.0f) {
            vertices[i].weights /= totalWeight;
        }
    }

    // --- Create OpenGL Buffers ---
    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glGenBuffers(1, &mesh.ebo);
    glBindVertexArray(mesh.vao);

    // VBO (AnimatedVertex)
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(AnimatedVertex), vertices.data(), GL_STATIC_DRAW);

    // EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // --- VBO Attrib Pointers ---
    // (Pos, Normal, UV)
    glEnableVertexAttribArray(0); // pos
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(AnimatedVertex), (void*)offsetof(AnimatedVertex, pos));
    glEnableVertexAttribArray(1); // normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(AnimatedVertex), (void*)offsetof(AnimatedVertex, normal));
    glEnableVertexAttribArray(2); // tex
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(AnimatedVertex), (void*)offsetof(AnimatedVertex, tex));

    // --- Attrib Pointers for Skinning ---
    glEnableVertexAttribArray(3); // boneIDs
    glVertexAttribIPointer( // Use 'I' for integers
        3, 4, GL_INT, sizeof(AnimatedVertex), (void*)offsetof(AnimatedVertex, boneIDs)
    );

    glEnableVertexAttribArray(4); // weights
    glVertexAttribPointer(
        4, 4, GL_FLOAT, GL_FALSE, sizeof(AnimatedVertex), (void*)offsetof(AnimatedVertex, weights)
    );

    glBindVertexArray(0);
    mesh.indexCount = static_cast<unsigned int>(indices.size());

    std::cout << "[MeshLoader] Loaded ANIMATED mesh: " << path << std::endl;
}