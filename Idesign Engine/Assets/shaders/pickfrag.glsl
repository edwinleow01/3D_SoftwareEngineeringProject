#version 430 core
layout(location = 0) out vec4 FragColor;

uniform vec3 uIdColor; // 0..1 packed color (RGB)

void main()
{
    // Use vec4 output for widest driver compatibility.
    // Your FBO is GL_RGB8, alpha will be ignored.
    FragColor = vec4(uIdColor, 1.0);
}
