#version 430 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aUV;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;
uniform mat4 uLightSpaceMatrix;

out vec3 vNormal;
out vec3 vWorldPos;
out vec4 v_FragPosLightSpace;
out vec2 vUV;

void main()
{
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    vWorldPos = worldPos.xyz;

    mat3 N = mat3(transpose(inverse(uModel)));
    vNormal = normalize(N * aNormal);

    v_FragPosLightSpace = uLightSpaceMatrix * worldPos;

    vUV = aUV;  // pass to frag shader

    gl_Position = uProj * uView * worldPos;
}
