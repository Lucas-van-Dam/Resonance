#version 460 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D sceneTexture;
uniform sampler2D bloomTexture;

uniform float bloomStrength;

void main() {
    vec3 sceneColor = texture(sceneTexture, TexCoords).rgb;
    vec3 bloomColor = texture(bloomTexture, TexCoords).rgb;

    FragColor = vec4 (sceneColor + bloomColor * bloomStrength, 1.0);
}