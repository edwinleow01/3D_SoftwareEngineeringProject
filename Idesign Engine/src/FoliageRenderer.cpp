#include "pch.h"
#include "FoliageRenderer.h"

FoliageRenderer::FoliageRenderer(
    Mesh* meshRef,
    const char* vsPath,
    const char* fsPath)
    : mesh(meshRef)
{
    // 1. Init Shader (Ensure you have the instanced.vert shader!)
    shader = new ShaderProgram(vsPath, fsPath);
    shader->initGL();

    // 2. Generate buffers for the matrices
    glGenBuffers(1, &instanceVBO);

    // 3. Create the Custom VAO for Instancing
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // --- BIND MESH DATA ---
    // Use the Getter since vbo is private
    glBindBuffer(GL_ARRAY_BUFFER, mesh->getVBO());

    // NOTE: We do NOT bind an EBO here because your Mesh class doesn't use one.

    // Set standard attributes (Position=0, Normal=1, UV=2)
    // Adjust these strides if your Mesh Vertex struct is different!
    // Assuming: float x, y, z, nx, ny, nz, u, v (8 floats total)
    size_t stride = 8 * sizeof(float);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));

    // If your mesh has UVs (check your mesh generation):
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));

    // --- BIND INSTANCE DATA ---
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);

    // Matrix is 4 vec4s (Attributes 3, 4, 5, 6)
    size_t vec4Size = sizeof(glm::vec4);
    for (int i = 0; i < 4; i++) {
        glEnableVertexAttribArray(3 + i);
        glVertexAttribPointer(3 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(i * vec4Size));

        // CRITICAL: Update once per INSTANCE
        glVertexAttribDivisor(3 + i, 1);
    }

    glBindVertexArray(0);
}

FoliageRenderer::~FoliageRenderer() {
    if (instanceVBO) glDeleteBuffers(1, &instanceVBO);
    if (vao) glDeleteVertexArrays(1, &vao);
    if (shader) {
        shader->destroy();
        delete shader;
    }
}

void FoliageRenderer::setInstances(const std::vector<glm::mat4>& transforms) {
    instanceCount = (int)transforms.size();
    if (instanceCount == 0) return;

    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, instanceCount * sizeof(glm::mat4), transforms.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void FoliageRenderer::draw(const glm::mat4& view, const glm::mat4& proj, const glm::mat4& model, unsigned int textureID) {
    if (instanceCount == 0) return;

    shader->use();
    shader->setMat4("uView", view);
    shader->setMat4("uProj", proj);
    shader->setMat4("uModel", model); // <--- SEND THE OFFSET TO GPU

    // === FIX: BIND TEXTURE ===
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID); // Use the ID passed in
    shader->setInt("uTexture", 0);
    // ========================

    // Optional: Set a dummy color or ambient light if needed by the shader
    shader->setVec3("uAmbientColor", glm::vec3(1.0f));

    glBindVertexArray(vao);

    // CRITICAL RE-BIND (fixes broken UVs, broken vertex fetch)
    glBindBuffer(GL_ARRAY_BUFFER, mesh->getVBO());

    glDrawArraysInstanced(GL_TRIANGLES, 0, mesh->getVertexCount(), instanceCount);

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0); // Cleanup texture binding
}