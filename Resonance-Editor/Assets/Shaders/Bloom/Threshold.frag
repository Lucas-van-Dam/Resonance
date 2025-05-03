#version 460 core

layout (binding = 0) uniform sampler2D srcTexture;
uniform float threshold = 1.1;

in vec2 TexCoords;
layout (location = 0) out vec3 FragColor;

void main() {
    vec3 srcColor = texture(srcTexture, TexCoords).rgb;
    float brightness = dot(srcColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    if (brightness >= threshold)
        FragColor = srcColor;
    else
        FragColor = vec3(0.0);
}