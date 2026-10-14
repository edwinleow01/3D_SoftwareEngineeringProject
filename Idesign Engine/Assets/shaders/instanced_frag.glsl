#version 430 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 Normal; // Assuming your vertex shader outputs a Normal

uniform sampler2D uTexture;
uniform vec3 uAmbientColor; // For basic flat color if no texture is desired
// Add your lighting uniforms here (e.g., uDirLight structure)

void main()
{
    // === Option 1: Textured (Best for trees) ===
    vec4 texColor = texture(uTexture, TexCoords);
    
    // Simple Unlit Color (for debugging or simple style)
    // FragColor = texColor; 
    
    // Simple Lit Color (use your full lighting model if available)
    // For now, use a simple ambient/diffuse calculation:
    
    // Dummy lighting calculation
    vec3 lightDir = normalize(vec3(0.0, -1.0, 0.0)); // Downwards light
    vec3 norm = normalize(Normal);
    float diff = max(dot(norm, -lightDir), 0.0);
    vec3 result = (uAmbientColor + diff) * texColor.rgb;
    
    FragColor = vec4(result, texColor.a);
    
    // === Option 2: Flat Color (For debugging the mesh itself) ===
    // FragColor = vec4(0.0, 1.0, 0.0, 1.0); // Green color
}