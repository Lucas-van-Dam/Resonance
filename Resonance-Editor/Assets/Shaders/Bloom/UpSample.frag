#version 460 core

layout (binding = 0) uniform sampler2D srcTexture;
layout (binding = 1) uniform sampler2D prevMip;
uniform float aspectRatio;
uniform float filterRadius;
uniform bool firstMip;
uniform vec2 texelSize;
uniform float mipWeight;

in vec2 TexCoords;
layout (location = 0) out vec3 upsample;

void main()
{
    vec3 color = texture(srcTexture, TexCoords).rgb;
    if(!firstMip) {
        float x = filterRadius * texelSize.x;
        float y = filterRadius * texelSize.y;

        vec3 a = texture(prevMip, vec2(TexCoords.x - x, TexCoords.y + y)).rgb;
        vec3 b = texture(prevMip, vec2(TexCoords.x,     TexCoords.y + y)).rgb;
        vec3 c = texture(prevMip, vec2(TexCoords.x + x, TexCoords.y + y)).rgb;

        vec3 d = texture(prevMip, vec2(TexCoords.x - x, TexCoords.y)).rgb;
        vec3 e = texture(prevMip, vec2(TexCoords.x,     TexCoords.y)).rgb;
        vec3 f = texture(prevMip, vec2(TexCoords.x + x, TexCoords.y)).rgb;

        vec3 g = texture(prevMip, vec2(TexCoords.x - x, TexCoords.y - y)).rgb;
        vec3 h = texture(prevMip, vec2(TexCoords.x,     TexCoords.y - y)).rgb;
        vec3 i = texture(prevMip, vec2(TexCoords.x + x, TexCoords.y - y)).rgb;

        vec3 blurred = e*4.0;
        blurred += (b+d+f+h)*2.0;
        blurred += (a+c+g+i);
        blurred *= 1.0 / 16.0;

        color += blurred * mipWeight;
    }

    upsample = color;
}