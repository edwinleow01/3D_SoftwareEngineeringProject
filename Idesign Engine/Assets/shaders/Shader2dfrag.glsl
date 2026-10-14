#version 430 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D uTexture; // Your 2D image
uniform vec3 uColor;        // Optional: To tint the image (usually white)
uniform float uAlpha;       // Optional: To fade the UI (transparency)
uniform float uGamma;       // Gamma correction (matches scene gamma slider)

void main()
{
    vec4 texColor = texture(uTexture, TexCoords);
    
    // Apply tint and global alpha
    FragColor = vec4(texColor.rgb * uColor, texColor.a * uAlpha);
    
    // Optimization: Don't draw invisible pixels
    if(FragColor.a < 0.01)
        discard;

    // Apply gamma correction to RGB only — alpha must not be affected
    FragColor.rgb = pow(FragColor.rgb, vec3(1.0 / uGamma));

    // Use a linear brightness multiply:
    // FragColor.rgb = FragColor.rgb * uGamma;
}
