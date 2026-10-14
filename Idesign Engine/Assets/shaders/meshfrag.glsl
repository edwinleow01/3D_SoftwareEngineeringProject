#version 430 core
out vec4 FragColor;

in vec3 vNormal;
in vec3 vWorldPos;
in vec2 vTex;
in vec4 v_FragPosLightSpace;

struct DirectionalLight {
    vec3 direction;
    vec3 color;
    float intensity;
    float shadowBiasConst;
    float shadowBiasFactor;
};

struct PointLight {
    vec3 position;
    vec3 color;
    float intensity;
    float radius;
    bool castsShadow;
    float shadowBiasConst;
    float shadowBiasFactor;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    vec3 color;
    float intensity;
    float radius;
    float innerCutoff;
    float outerCutoff;
};

struct Material {
    vec3 ambient;
    vec3 specular;
    float shininess;
    float opacity;
};

#define MAX_POINT_LIGHTS 16
#define MAX_SPOT_LIGHTS  4

uniform DirectionalLight uDirLight;
uniform bool uHasDirLight;

uniform int uNumPointLights;
uniform PointLight uPointLights[MAX_POINT_LIGHTS];

uniform int uNumSpotLights;
uniform SpotLight uSpotLights[MAX_SPOT_LIGHTS];

uniform samplerCube uPointShadowMaps[MAX_POINT_LIGHTS];

// FIX 1: Each point light's shadow far plane is uploaded separately so the
// depth reconstruction matches exactly what the shadow-pass wrote.
// (Previously the shader used uPointLights[i].radius, but Graphics.cpp wrote
//  depth normalised against shadowFarPlane = max(radius, 50), so they never matched.)
uniform float uPointShadowFarPlanes[MAX_POINT_LIGHTS];

uniform vec3 uAlbedo;
uniform sampler2D uTexture;
uniform vec3 uViewPos;
uniform Material uMaterial;
uniform sampler2D uShadowMap;

// --- POINT SHADOW FUNCTION ---
float CalculatePointShadow(int index, vec3 L, vec3 N, vec3 Lnorm)
{
    // L  = lightPos - fragPos  (points toward the light)
    // Sample the cubemap in the opposite direction (light → fragment)
    vec3 lightToFrag = -L;

    float closestNormalizedDepth = texture(uPointShadowMaps[index], lightToFrag).r;

    // Reconstruct world-space depth using the exact far plane used during the shadow pass
    float closestDepth = closestNormalizedDepth * uPointShadowFarPlanes[index];

    // currentDepth = true world-space distance from light to this fragment
    float currentDepth = length(L);

    // Early-out: cubemap face was cleared to 1.0 = no occluder recorded = fully lit
    if (closestNormalizedDepth >= 1.0)
        return 0.0;

    // ACNE FIX: slope-scaled bias.
    // A flat bias (e.g. 0.15) causes Peter-Panning on steep faces and still
    // acnes on shallow ones.  Scale the bias by the angle between the surface
    // normal and the light direction so steep surfaces get more bias.
    float cosTheta = clamp(dot(N, Lnorm), 0.0, 1.0);
    float slopeBias = uPointLights[index].shadowBiasFactor * tan(acos(cosTheta));
    slopeBias = clamp(slopeBias, 0.0, 0.5);
    float bias = uPointLights[index].shadowBiasConst + slopeBias;

    return (currentDepth - bias) > closestDepth ? 1.0 : 0.0;
}

float CalculateShadow(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    // 1. Perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // 2. Transform from [-1,1] to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;

    // 3. Get closest depth from shadow map
    float closestDepth = texture(uShadowMap, projCoords.xy).r;

    // 4. Get current fragment's depth
    float currentDepth = projCoords.z;

    // 5. Check if outside the shadow map (should be lit)
    if (projCoords.z > 1.0)
        return 0.0;

    // 6. Fix "shadow acne" with a bias based on surface angle
    float bias = max(uDirLight.shadowBiasFactor * (1.0 - dot(normal, lightDir)),
        uDirLight.shadowBiasConst);

    // 7. PCF (Percentage-Closer Filtering) for soft shadows
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(uShadowMap, 0);
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(uShadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += (currentDepth - bias) > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

    return shadow;
}

void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uViewPos - vWorldPos); // View direction
    float shadow = 0.0;

    vec3 totalDiffuse = vec3(0.0);
    vec3 totalSpecular = vec3(0.0);

    // Directional Light
    if (uHasDirLight && uDirLight.intensity > 0.0) {
        vec3 Ld = normalize(-uDirLight.direction);
        shadow = CalculateShadow(v_FragPosLightSpace, N, Ld);
        float diff = max(dot(N, Ld), 0.0);
        totalDiffuse += uDirLight.color * uDirLight.intensity * diff * (1.0 - shadow);
        vec3 H = normalize(Ld + V);
        float spec = pow(max(dot(N, H), 0.0), uMaterial.shininess);
        totalSpecular += uMaterial.specular * spec * uDirLight.color * uDirLight.intensity * (1.0 - shadow);
    }

    // Point Lights
    for (int i = 0; i < uNumPointLights; ++i) {
        if (uPointLights[i].intensity <= 0.0 || uPointLights[i].radius <= 0.0) continue;

        vec3 L = uPointLights[i].position - vWorldPos;
        float dist = length(L);

        if (dist < uPointLights[i].radius) {
            vec3 Lnorm = normalize(L);
            float atten = 1.0 - (dist / uPointLights[i].radius);

            float ptShadow = 0.0;
            if (uPointLights[i].castsShadow)
            {
                ptShadow = CalculatePointShadow(i, L, N, Lnorm);
            }

            // Diffuse
            float d = max(dot(N, Lnorm), 0.0);
            totalDiffuse += uPointLights[i].color * uPointLights[i].intensity * d * atten * (1.0 - ptShadow);
            // Specular (Blinn-Phong)
            vec3 H = normalize(Lnorm + V);
            float spec = pow(max(dot(N, H), 0.0), uMaterial.shininess);
            totalSpecular += uMaterial.specular * spec * uPointLights[i].color * uPointLights[i].intensity * atten * (1.0 - ptShadow);
        }
    }

    // Spot Lights
    for (int i = 0; i < uNumSpotLights; ++i) {
        if (uSpotLights[i].intensity <= 0.0 || uSpotLights[i].radius <= 0.0) continue;

        vec3 L = uSpotLights[i].position - vWorldPos;
        float dist = length(L);

        if (dist < uSpotLights[i].radius) {
            vec3 Lnorm = normalize(L);
            float theta = dot(Lnorm, normalize(-uSpotLights[i].direction));
            float epsilon = uSpotLights[i].innerCutoff - uSpotLights[i].outerCutoff;
            float spotFactor = clamp((theta - uSpotLights[i].outerCutoff) / epsilon, 0.0, 1.0);
            float atten = 1.0 - (dist / uSpotLights[i].radius);

            // Diffuse
            float d = max(dot(N, Lnorm), 0.0);
            totalDiffuse += uSpotLights[i].color * uSpotLights[i].intensity * d * atten * spotFactor;

            // Specular (Blinn-Phong)
            vec3 H = normalize(Lnorm + V);
            float spec = pow(max(dot(N, H), 0.0), uMaterial.shininess);
            totalSpecular += uMaterial.specular * spec * uSpotLights[i].color * uSpotLights[i].intensity * d * atten * spotFactor;
        }
    }

    // --- FINAL COMBINATION ---
    vec3 texColor = texture(uTexture, vTex).rgb;
    vec3 albedoColor = texColor * uAlbedo;

    vec3 result = uMaterial.ambient + (totalDiffuse * albedoColor) + totalSpecular;

    FragColor = vec4(result * uMaterial.opacity, uMaterial.opacity);
}
