#version 430 core
in vec3 vWorldPos;

uniform vec3 uLightPos;
uniform float uFarPlane; 

out float FragColor; // MUST BE DECLARED

void main()
{
    // Calculate distance from fragment to light's center
    float lightDist = length(vWorldPos - uLightPos);
    
    // Normalize this distance to [0, 1] range
    FragColor = lightDist / uFarPlane;

}