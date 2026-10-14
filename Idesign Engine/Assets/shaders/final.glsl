#version 430 core
out vec4 FragColor;
in vec2 vTexCoords;

uniform sampler2D sceneTexture;
uniform sampler2D bloomTexture;
uniform float exposure;
uniform float bloomStrength;
uniform float gamma;

void main()
{
    vec3 hdrColor = texture(sceneTexture, vTexCoords).rgb;
    vec3 bloomColor = texture(bloomTexture, vTexCoords).rgb;

    // Additively blend bloom effect with the scene
    hdrColor += bloomColor * bloomStrength;

    // Reinhard tone mapping to map HDR colors to the display's [0,1] range
    vec3 mappedColor = hdrColor / (hdrColor + vec3(1.0));

    // Apply exposure control
    mappedColor = vec3(1.0) - exp(-hdrColor * exposure);
    
    // Apply Gamma Correction
    mappedColor = pow(mappedColor, vec3(1.0 / gamma));

    FragColor = vec4(mappedColor, 1.0);
}