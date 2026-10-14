#version 430 core
out vec4 FragColor;

in vec3 vNormal;
in vec3 vWorldPos;

uniform vec3 uLightDir;  // should be normalized, in world space
uniform vec3 uAlbedo;

void main() {
    // simple Lambert with a tiny ambient
    float NdotL = max(dot(normalize(vNormal), normalize(-uLightDir)), 0.0);
    vec3 color = uAlbedo * (0.15 + 0.85 * NdotL);
    FragColor = vec4(color, 1.0);
}
