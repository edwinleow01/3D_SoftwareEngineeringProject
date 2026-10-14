#version 430 core

// INPUT: From your Vertex Array Object (VAO)
layout(location = 0) in vec3 aPos;

// OPTIONAL: Declare but don't use attributes 3 & 4
// This prevents shader compilation warnings
// layout(location=3) in ivec4 aBoneIDs;  // Don't declare - not used!
// layout(location=4) in vec4 aWeights;   // Don't declare - not used!

// OUTPUT: To the point_shadow_frag.glsl
out vec3 vWorldPos;

// UNIFORMS: From your Graphics.cpp
uniform mat4 uModel;
uniform mat4 uLightSpaceMatrix;

void main()
{
    // Simple non-skinned vertex transformation
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    vWorldPos = worldPos.xyz;
    gl_Position = uLightSpaceMatrix * worldPos;
}