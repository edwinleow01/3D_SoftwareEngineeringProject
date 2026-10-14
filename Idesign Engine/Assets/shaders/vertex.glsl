#version 430 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

out vec3 vNormal;
out vec3 vWorldPos;

void main() {
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    vWorldPos = worldPos.xyz;

    // normal matrix: inverse-transpose of upper-left 3x3 of model
    mat3 N = mat3(transpose(inverse(uModel)));
    vNormal = normalize(N * aNormal);

    gl_Position = uProj * uView * worldPos;
}
