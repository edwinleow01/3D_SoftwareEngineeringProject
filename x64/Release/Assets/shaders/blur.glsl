#version 430 core
out vec4 FragColor;
in vec2 vTexCoords;

uniform sampler2D image;
uniform bool horizontal;
float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main()
{
    vec3 color = texture(image, vTexCoords).rgb;

    // Bright-pass filter: only keep colors with a brightness > 1.0
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
    if(brightness <1.0) {
        color = vec3(0.0);
    }

    // Gaussian blur
    vec2 tex_offset = 1.0 / textureSize(image, 0);
    vec3 result = color * weight[0];
    if(horizontal) {
        for(int i = 1; i < 5; ++i) {
            result += texture(image, vTexCoords + vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
            result += texture(image, vTexCoords - vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
        }
    } else {
        for(int i = 1; i < 5; ++i) {
            result += texture(image, vTexCoords + vec2(0.0, tex_offset.y * i)).rgb * weight[i];
            result += texture(image, vTexCoords - vec2(0.0, tex_offset.y * i)).rgb * weight[i];
        }
    }
    FragColor = vec4(result, 1.0);
}