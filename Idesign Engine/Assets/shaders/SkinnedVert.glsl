#version 430 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTex;
layout(location = 3) in ivec4 aBoneIDs;
layout(location = 4) in vec4 aWeights;

out vec3 vNormal;   // Was 'Normal'
out vec3 vWorldPos; // Was 'FragPos'
out vec2 vTex;      // Was 'TexCoords'
out vec4 v_FragPosLightSpace;

uniform mat4 uProj;
uniform mat4 uView;
uniform mat4 uModel;

#define MAX_BONES 100
uniform mat4 uBoneMatrices[MAX_BONES];
uniform mat4 uLightSpaceMatrix;

void main()
{
    mat4 skinTransform;
    mat3 normalSkinTransform;


    // A vertex with no bones will have aBoneIDs.x == -1
    if(aBoneIDs.x == -1)
    {
        skinTransform = mat4(1.0); // Not skinned, use identity
        
        normalSkinTransform = mat3(1.0); // Use identity for normal as well
  
    }
    else
    {
        // This vertex is skinned. Reset to 0 and sum weights.
        skinTransform = mat4(0.0);

        normalSkinTransform = mat3(0.0); // Reset normal matrix


        // --- Check EACH bone ID before indexing ---
        skinTransform += uBoneMatrices[aBoneIDs.x] * aWeights.x;
        
        normalSkinTransform += mat3(uBoneMatrices[aBoneIDs.x]) * aWeights.x;


        if(aBoneIDs.y > -1)
        {
            skinTransform += uBoneMatrices[aBoneIDs.y] * aWeights.y;
            normalSkinTransform += mat3(uBoneMatrices[aBoneIDs.y]) * aWeights.y;
        }
            
        if(aBoneIDs.z > -1)
        {
            skinTransform += uBoneMatrices[aBoneIDs.z] * aWeights.z;

            normalSkinTransform += mat3(uBoneMatrices[aBoneIDs.z]) * aWeights.z;
    
        }
            
        if(aBoneIDs.w > -1)
        {
            skinTransform += uBoneMatrices[aBoneIDs.w] * aWeights.w;
    
            normalSkinTransform += mat3(uBoneMatrices[aBoneIDs.w]) * aWeights.w;
         
        }
    }

    vec4 localPos = skinTransform * vec4(aPos, 1.0);
    
 
    // 1. Get the normal matrix for the object's main world transform
    //    (This handles non-uniform scaling of the whole object)
    mat3 normalMatrix = mat3(transpose(inverse(uModel)));
    
    // 2. Transform vertex to world space
    vec4 worldPos = uModel * localPos;

    // 3. Apply both the world normal matrix AND the blended skin normal matrix
    vNormal = normalize(normalMatrix * normalSkinTransform * aNormal);
 

    vWorldPos = worldPos.xyz;
    vTex = aTex;
    v_FragPosLightSpace = uLightSpaceMatrix * worldPos; 
    gl_Position = uProj * uView * worldPos;
}