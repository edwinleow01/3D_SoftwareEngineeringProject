#version 430 core
layout (location = 0) in vec2 aPos;       // Vertex Position (x, y)
layout (location = 1) in vec2 aTexCoords; // Texture Coordinates (u, v)

out vec2 TexCoords;

uniform mat4 uProjection; // Orthographic projection (pixels)
uniform mat4 uModel;      // Position and size of the image

void main()
{
    TexCoords = aTexCoords;
    // Z is 0.0 because UI is flat. W is 1.0 for standard math.
    gl_Position = uProjection * uModel * vec4(aPos, 0.0, 1.0); 
}