#version 430 core

out vec4 FragColor;

in vec2 TexCoords;
in vec3 Normal;

uniform sampler2D uTexture;

void main()
{
    vec4 c = texture(uTexture, TexCoords);
    if (c.a < 0.2) discard;

    vec3 lightDir = normalize(vec3(0.3, -1.0, 0.2));
    float diff = max(dot(normalize(Normal), -lightDir), 0.0);

    FragColor = vec4(c.rgb * (0.4 + diff), c.a);
}
