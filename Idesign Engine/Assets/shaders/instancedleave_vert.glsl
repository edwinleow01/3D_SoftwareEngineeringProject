#version 430 core

layout (location = 0) in vec3 aPos;
layout (location = 2) in vec2 aUV;
layout (location = 3) in mat4 aInstance;

out vec2 TexCoords;

uniform mat4 uProj;
uniform mat4 uView;
uniform mat4 uModel;  // <<<<<<<<< ADD THIS

mat3 ExtractCameraRotation(mat4 view)
{
    mat3 rot;
    rot[0] = vec3(view[0][0], view[1][0], view[2][0]);
    rot[1] = vec3(view[0][1], view[1][1], view[2][1]);
    rot[2] = vec3(view[0][2], view[1][2], view[2][2]);
    return rot;
}

void main()
{
    TexCoords = aUV;

    vec3 treePos = vec3(aInstance[3]);

    float leafWidth  = 3.0;
    float leafHeight = 4.5;
    float trunkHeight = 0.5;

    vec3 local = vec3(
        aPos.x * leafWidth,
        aPos.y * leafHeight + trunkHeight,
        0.0
    );

    mat3 camRot = ExtractCameraRotation(uView);
    vec3 worldPos = treePos + camRot * local;

    // <<<<<< APPLY GLOBAL MODEL TRANSFORM
    vec4 finalPos = uModel * vec4(worldPos, 1.0);

    gl_Position = uProj * uView * finalPos;
}
