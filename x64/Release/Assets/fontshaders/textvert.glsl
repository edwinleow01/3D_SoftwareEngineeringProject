#version 430 core
layout (location = 0) in vec4 vertex; // pos.xy, texcoord.xy
out vec2 TexCoord;

uniform mat4 projection;
uniform mat4 model; // identity for 2D text

void main()
{
    gl_Position = projection * model * vec4(vertex.xy, 0.0, 1.0);
    TexCoord = vertex.zw;
}