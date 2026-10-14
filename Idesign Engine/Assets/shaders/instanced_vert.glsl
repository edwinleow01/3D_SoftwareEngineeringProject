#version 430 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in mat4 aInstanceMatrix; 

out vec2 TexCoords;
out vec3 Normal;

uniform mat4 uProj;
uniform mat4 uView;
uniform mat4 uModel;

void main()
{
    TexCoords = aTexCoords;
    
    // Apply Global Model Matrix * Instance Matrix
    gl_Position = uProj * uView * uModel * aInstanceMatrix * vec4(aPos, 1.0);
    
    // Rotate normal by the combined rotation
    Normal = mat3(transpose(inverse(uModel * aInstanceMatrix))) * aNormal; 
}