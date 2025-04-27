#version 460 core

layout (binding = 0) uniform sampler2D originalColor;
layout (binding = 1) uniform sampler2D blurredColor;
layout (binding = 2) uniform sampler2D cocTexture;

in vec2 TexCoords;
layout (location = 0) out vec4 finalColor;

void main() {
    float coc = abs(texture(cocTexture, TexCoords).r);

    vec3 sharp = texture(originalColor, TexCoords).rgb;
    vec3 blur = texture(blurredColor, TexCoords).rgb;

    finalColor = vec4(mix(sharp, blur, coc), 1.0);
}