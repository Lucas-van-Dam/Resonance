#version 460 core

out vec4 FragColor;

in vec2 TexCoords;

layout(binding = 0) uniform sampler2D scene;
layout(binding = 1) uniform sampler2D bloomBlur;
uniform float bloomStrength = 0.04f;

void main() {
    vec3 hdrColor = texture(scene, TexCoords).rgb;
    vec3 bloomColor = texture(bloomBlur, TexCoords).rgb;
    // Add bloom on top
    vec3 color = hdrColor + bloomColor;

    FragColor = vec4(color, 1.0);
}