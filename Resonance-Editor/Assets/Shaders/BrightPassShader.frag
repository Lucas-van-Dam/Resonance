#version 460 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform float threshold;

void main() {
    vec3 color = texture(screenTexture, TexCoords).rgb;

    if(length(color) > threshold)
            FragColor = vec4(color, 1.0);
    else
        discard;
}