#version 430 core

layout (location = 0) in vec3 aPos;
layout (location = 2) in vec2 aUV;
layout (location = 3) in mat4 aInstance;

out vec2 TexCoords;

uniform mat4 uProj;
uniform mat4 uView;
uniform mat4 uModel;   // keep for compatibility with your renderer

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

    // Translation from instance matrix
    vec3 pos = vec3(aInstance[3]);

    // Extract scale from instance matrix columns (works with glm::scale)
    float sx = length(vec3(aInstance[0]));
    float sy = length(vec3(aInstance[1]));

    // Quad in local space (centered billboard)
    vec3 local = vec3(aPos.x * sx, aPos.y * sy, 0.0);

    // Billboard using camera rotation (faces camera)
    mat3 camRot = ExtractCameraRotation(uView);
    vec3 worldPos = pos + camRot * local;

    vec4 finalPos = uModel * vec4(worldPos, 1.0);
    gl_Position = uProj * uView * finalPos;
}
