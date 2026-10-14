#version 430 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal; // unused, keeps VAO layout consistent
layout(location = 2) in vec2 aUV;     // unused

uniform mat4 uProj, uView, uModel;

void main()
{
    gl_Position = uProj * uView * uModel * vec4(aPos, 1.0);
}
