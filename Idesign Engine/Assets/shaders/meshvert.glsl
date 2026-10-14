#version 430 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aTex;   // UVs

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;
uniform mat4 uLightSpaceMatrix;

out vec3 vNormal;
out vec3 vWorldPos;
out vec2 vTex;
out vec4 v_FragPosLightSpace;

void main() {
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    vWorldPos = worldPos.xyz;

    mat3 N = mat3(transpose(inverse(uModel)));
    vNormal = normalize(N * aNormal);

    vTex = aTex;
    v_FragPosLightSpace = uLightSpaceMatrix * worldPos;
    gl_Position = uProj * uView * worldPos;
}
