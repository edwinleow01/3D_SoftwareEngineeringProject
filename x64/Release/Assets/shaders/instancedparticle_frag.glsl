#version 430 core

out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D uTexture;

void main()
{
    vec4 c = texture(uTexture, TexCoords);
    if (c.a < 0.2) discard;
    FragColor = c;
}
