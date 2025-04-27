#version 460 core

uniform sampler2D srcTexture;
uniform vec2 srcTexelSize[5];
uniform int uMipLevel;

in vec2 TexCoords;
layout (location = 0) out vec3 downsample;

void main() {
    float x = srcTexelSize[uMipLevel].x;
    float y = srcTexelSize[uMipLevel].y;

    vec3 a = texture(srcTexture, vec2(TexCoords.x - 2*x, TexCoords.y + 2*y)).rgb;
    vec3 b = texture(srcTexture, vec2(TexCoords.x,       TexCoords.y + 2*y)).rgb;
    vec3 c = texture(srcTexture, vec2(TexCoords.x + 2*x, TexCoords.y + 2*y)).rgb;

    vec3 d = texture(srcTexture, vec2(TexCoords.x - 2*x, TexCoords.y)).rgb;
    vec3 e = texture(srcTexture, vec2(TexCoords.x,       TexCoords.y)).rgb;
    vec3 f = texture(srcTexture, vec2(TexCoords.x + 2*x, TexCoords.y)).rgb;

    vec3 g = texture(srcTexture, vec2(TexCoords.x - 2*x, TexCoords.y - 2*y)).rgb;
    vec3 h = texture(srcTexture, vec2(TexCoords.x,       TexCoords.y - 2*y)).rgb;
    vec3 i = texture(srcTexture, vec2(TexCoords.x + 2*x, TexCoords.y - 2*y)).rgb;

    vec3 j = texture(srcTexture, vec2(TexCoords.x - x, TexCoords.y + y)).rgb;
    vec3 k = texture(srcTexture, vec2(TexCoords.x + x, TexCoords.y + y)).rgb;
    vec3 l = texture(srcTexture, vec2(TexCoords.x - x, TexCoords.y - y)).rgb;
    vec3 m = texture(srcTexture, vec2(TexCoords.x + x, TexCoords.y - y)).rgb;

    downsample = e*0.125;
    downsample += (a+c+g+i)*0.03125;
    downsample += (b+d+f+h)*0.0625;
    downsample += (j+k+l+m)*0.125;
    downsample = max(downsample, 0.0001f);
}