#version 460 core

layout (binding = 0) uniform sampler2D inputColor;
layout (binding = 1) uniform sampler2D cocTexture;

uniform vec2 texelSize;

in vec2 TexCoords;
layout (location = 0) out vec4 blurredColor;

void main() {
    float coc = texture(cocTexture, TexCoords).r;
    coc = abs(coc); // Always positive

    float radius = coc * 15.0; // Max radius (adjustable, e.g., 15 px)

    vec3 colorSum = vec3(0.0);
    float weightSum = 0.0;

    // Choose how many steps you want for quality/performance
    const int NUM_STEPS = 10;

    for (int i = 0; i < NUM_STEPS; ++i)
    {
        // Random angle around circle
        float angle = (float(i) / float(NUM_STEPS)) * 6.2831853; // 2*pi
        vec2 dir = vec2(cos(angle), sin(angle));

        vec2 offset = dir * radius * texelSize;
        vec2 sampleCoord = TexCoords + offset;

        vec3 sampleColor = texture(inputColor, sampleCoord).rgb;

        colorSum += sampleColor;
        weightSum += 1.0;
    }

    blurredColor = vec4(colorSum / weightSum, 1.0);
}