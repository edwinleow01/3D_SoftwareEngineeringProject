#version 430 core
out vec4 FragColor;

in vec3 vNormal;
in vec3 vWorldPos;

struct DirLight {
    vec3 direction;
    vec3 color;
    float intensity;
};

struct PointLight {
    vec3 position;
    vec3 color;
    float intensity;
    float radius;
};

#define MAX_POINT_LIGHTS 8
uniform DirLight uDirLight;
uniform int uNumPointLights;
uniform PointLight uPointLights[MAX_POINT_LIGHTS];

uniform vec3 uAlbedo;

void main() {
    vec3 N = normalize(vNormal);
    vec3 result = vec3(0.0);

    // Directional light
    vec3 Ld = normalize(-uDirLight.direction);
    float diff = max(dot(N, Ld), 0.0);
    result += uDirLight.color * uDirLight.intensity * diff;

    // Point lights
    for (int i = 0; i < uNumPointLights; ++i) {
        vec3 L = uPointLights[i].position - vWorldPos;
        float dist = length(L);
        if (dist < uPointLights[i].radius) {
            vec3 Lnorm = normalize(L);
            float atten = 1.0 - (dist / uPointLights[i].radius);
            float d = max(dot(N, Lnorm), 0.0);
            result += uPointLights[i].color * uPointLights[i].intensity * d * atten;
        }
    }

    FragColor = vec4(uAlbedo * result, 1.0);
}
