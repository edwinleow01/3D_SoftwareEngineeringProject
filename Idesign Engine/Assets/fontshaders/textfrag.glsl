#version 430 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D text;
uniform vec3 textColor;
uniform float opacity;

void main()
{
    // Get alpha from font texture (Red channel)
    float fontAlpha = texture(text, TexCoord).r;
    
    // SYSTEMATIC FIX: Throw away invisible pixels so they don't 
    // block things behind them in the Depth Buffer!
    if(fontAlpha < 0.01) {
        discard; 
    }

    // Combine font alpha with global opacity
    FragColor = vec4(textColor, fontAlpha * opacity);
}