#version 430 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 3) in ivec4 aBoneIDs;
layout(location = 4) in vec4 aWeights;

out vec3 vWorldPos;
out vec3 vDebugColor; // Add this for debugging

uniform mat4 uModel;
uniform mat4 uLightSpaceMatrix;
uniform mat4 uBoneMatrices[100];

void main() {
    mat4 skinTransform = mat4(0.0);

    // DEBUG: Visualize bone IDs
    vDebugColor = vec3(0.0, 1.0, 0.0); // Green = OK

    if (aBoneIDs.x == -1) {
        skinTransform = mat4(1.0);
        vDebugColor = vec3(0.0, 0.0, 1.0); // Blue = no skinning
    }
    else if (aBoneIDs.x < 0 || aBoneIDs.x >= 100) {
        // INVALID BONE ID!
        skinTransform = mat4(1.0);
        vDebugColor = vec3(1.0, 0.0, 0.0); // Red = ERROR
    }
    else {
        skinTransform += uBoneMatrices[aBoneIDs.x] * aWeights.x;
        if (aBoneIDs.y > -1 && aBoneIDs.y < 100) skinTransform += uBoneMatrices[aBoneIDs.y] * aWeights.y;
        if (aBoneIDs.z > -1 && aBoneIDs.z < 100) skinTransform += uBoneMatrices[aBoneIDs.z] * aWeights.z;
        if (aBoneIDs.w > -1 && aBoneIDs.w < 100) skinTransform += uBoneMatrices[aBoneIDs.w] * aWeights.w;

        vDebugColor = vec3(0.0, 1.0, 0.0); // Green = valid skinning
    }

    vec4 localPos = skinTransform * vec4(aPos, 1.0);
    vec4 worldPos = uModel * localPos;

    vWorldPos = worldPos.xyz;
    gl_Position = uLightSpaceMatrix * worldPos;
}