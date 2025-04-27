#version 460 core

layout (binding = 0) uniform sampler2D depthTexture;

uniform float focusDistance;
uniform float focusRange;
uniform float nearPlane = 0.1f;
uniform float farPlane = 100.0f;

in vec2 TexCoords;
layout (location = 0) out float coc;

float LinearizeDepth(float depth){
    float z = depth * 2.0 - 1.0;
    return (2.0 * nearPlane - farPlane) / (farPlane + nearPlane - z * (farPlane - nearPlane));
}

void main() {
    float depth = texture(depthTexture, TexCoords).r;
    float linearDepth = LinearizeDepth(depth);

    float cocValue = (linearDepth - focusDistance) / focusRange;
    coc = clamp(cocValue, -1.0, 1.0);
}