#version 430 core
out vec4 FragColor;

in vec3 TexCoords;

// We now have two textures
uniform samplerCube skyboxDay;
uniform samplerCube skyboxNight;

// 0.0 = Fully Day, 1.0 = Fully Night, 0.5 = Blend
uniform float blendFactor; 

void main()
{    
    vec4 dayColor = texture(skyboxDay, TexCoords);
    vec4 nightColor = texture(skyboxNight, TexCoords);
    
    // Linearly interpolate between day and night
    FragColor = mix(dayColor, nightColor, blendFactor);
}