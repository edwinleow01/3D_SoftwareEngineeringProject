#version 430 core
out vec4 FragColor;

in vec3 vNormal;
in vec3 vWorldPos;
in vec4 v_FragPosLightSpace;
in vec2 vUV;

uniform sampler2D uGrassTex;
uniform float uGrassTiling;

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

#define MAX_POINT_LIGHTS 8
#define MAX_SPOT_LIGHTS  4

uniform DirectionalLight uDirLight;
uniform bool uHasDirLight;

uniform int uNumPointLights;
uniform PointLight uPointLights[MAX_POINT_LIGHTS];

uniform int uNumSpotLights;
uniform SpotLight uSpotLights[MAX_SPOT_LIGHTS];

uniform samplerCube uPointShadowMaps[MAX_POINT_LIGHTS];

uniform vec3 uAlbedo;   
uniform sampler2D uShadowMap;

float CalculatePointShadow(int index, vec3 L, vec3 N, vec3 Lnorm)
{
    // 1. Sample the cubemap using the direction vector L
    //    .r contains the normalized distance [0,1]
    float closestNormalizedDepth = texture(uPointShadowMaps[index], L).r;
    if (closestNormalizedDepth <= 0.001 || closestNormalizedDepth >= 1.0)
    {
        return 0.0; // Not in shadow
    }
    // 2. Un-normalize it back to world-space distance
    float closestDepth = closestNormalizedDepth * uPointLights[index].radius;
    
    // 3. Get the *current* fragment's distance from the light
    float currentDepth = length(L);
    
    // 4. Add a bias to fix shadow acne
    //    (Point light bias is tricky, start with a small constant)
    float bias = max(uPointLights[index].shadowBiasFactor * (1.0 - dot(N, Lnorm)), 
                     uPointLights[index].shadowBiasConst);
    // 5. Compare
    float shadow = (currentDepth - bias) > closestDepth ?
1.0 : 0.0;
        
    return shadow;
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
    if(projCoords.z > 1.0)
        return 0.0; // 0.0 shadow = fully lit

    // 6. Fix "shadow acne" with a bias based on surface angle
   float bias = max(uDirLight.shadowBiasFactor * (1.0 - dot(normal, lightDir)), 
                     uDirLight.shadowBiasConst);
    
    // 7. PCF (Percentage-Closer Filtering) for soft shadows
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(uShadowMap, 0); // e.g., 1.0 / 2048.0
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(uShadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += (currentDepth - bias) > pcfDepth ? 1.0 : 0.0; // 1.0 = in shadow
        }    
    }
    shadow /= 9.0;
    
    return shadow; // 0.0 = no shadow, 1.0 = full shadow
}

void main() {


    vec3 N = normalize(vNormal);
    vec3 result = vec3(0.1); // baseline ambient

    // Directional
    if (uHasDirLight && uDirLight.intensity > 0.0) {
        vec3 Ld = normalize(-uDirLight.direction);
        float diff = max(dot(N, Ld), 0.0);

        // Calculate shadow
        float shadow = CalculateShadow(v_FragPosLightSpace, N, Ld);
        
        // Apply shadow to directional light
        result += uDirLight.color * uDirLight.intensity * diff * (1.0 - shadow);
    }

  // Point lights
    for (int i = 0; i < uNumPointLights; ++i) {
    if (uPointLights[i].intensity <= 0.0 || uPointLights[i].radius <= 0.0) continue;

    vec3 L = uPointLights[i].position - vWorldPos; // Vector from frag to light
    float dist = length(L);

    if (dist < uPointLights[i].radius) {
        vec3 Lnorm = normalize(L);
        float atten = 1.0 - (dist / uPointLights[i].radius);

        float shadow = 0.0;
        if(uPointLights[i].castsShadow)
        {
            shadow = CalculatePointShadow(i, L, N, Lnorm);
        }

        float d = max(dot(N, Lnorm), 0.0);
        result += uPointLights[i].color * uPointLights[i].intensity * d * atten * (1.0 - shadow); // MODIFIED
    }
}

    // Spot lights
    for (int i = 0; i < uNumSpotLights; ++i) {
        if (uSpotLights[i].intensity <= 0.0 || uSpotLights[i].radius <= 0.0) continue;

        vec3 L = uSpotLights[i].position - vWorldPos;
        float dist = length(L);
        if (dist < uSpotLights[i].radius) {
            vec3 Lnorm = normalize(L);
            float theta = dot(Lnorm, normalize(-uSpotLights[i].direction));
            float epsilon = uSpotLights[i].innerCutoff - uSpotLights[i].outerCutoff;
            float spotFactor = clamp((theta - uSpotLights[i].outerCutoff) / epsilon, 0.0, 1.0);

            float d = max(dot(N, Lnorm), 0.0);
            float atten = 1.0 - (dist / uSpotLights[i].radius);
            result += uSpotLights[i].color * uSpotLights[i].intensity * d * atten * spotFactor;
        }
    }

    vec3 grassColor = texture(uGrassTex, vUV * uGrassTiling).rgb;

    FragColor = vec4(grassColor * result, 1.0);
}
