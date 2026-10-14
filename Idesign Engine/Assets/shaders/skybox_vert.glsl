#version 430 core
layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 uProj;
uniform mat4 uView;

void main()
{
    TexCoords = aPos;
    // Convert view matrix to mat3 to remove translation (movement), 
    // keeping only rotation
    vec4 pos = uProj * mat4(mat3(uView)) * vec4(aPos, 1.0);
    
    // Optimization: set z to w so that z/w = 1.0 (maximum depth)
    gl_Position = pos.xyww;
}