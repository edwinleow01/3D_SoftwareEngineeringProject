#version 430 core

layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aUV;
layout(location=3) in mat4 aInstance;

out vec2 TexCoords;
out vec3 Normal;

uniform mat4 uProj;
uniform mat4 uView;
uniform mat4 uModel;

void main()
{
    TexCoords = aUV;
    Normal = mat3(uModel * aInstance) * aNormal;

    gl_Position = uProj * uView * uModel * aInstance * vec4(aPos, 1.0);
}
