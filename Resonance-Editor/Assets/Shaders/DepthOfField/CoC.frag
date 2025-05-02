#version 460 core

layout (binding = 0) uniform sampler2D depthTexture;

uniform float focusDistance = 10.0f;
uniform float focusRange = 1.0f;
uniform float nearPlane = 0.1f;
uniform float farPlane = 100.0f;

in vec2 TexCoords;
layout (location = 0) out float coc;

float LinearizeDepth(float depth){
    float z_n = 2.0 * depth - 1.0;
    return 2.0 * nearPlane * farPlane / (farPlane + nearPlane - z_n * (farPlane - nearPlane));
}

void main() {
    float depth = texture(depthTexture, TexCoords).r;

    if (depth <= 0.0 || depth >= 1.0) {
        coc = 0.0;
        return;
    }

    float linearDepth = LinearizeDepth(depth);

    float cocValue = (linearDepth - focusDistance) / linearDepth;
    coc = clamp(cocValue, -1.0, 1.0);
}