#version 460 core

out vec4 FragColor;

in vec2 TexCoords;

layout(binding = 0) uniform sampler2D screenTexture;



void main()
{
    vec3 hdrColor = texture(screenTexture, TexCoords).rgb;

    // tone mapping
    vec3 mapped = vec3(1.0) - exp(-hdrColor);
    // also gamma correct while we're at it
    const float gamma = 2.2;
    mapped = pow(mapped, vec3(1.0 / gamma));

    FragColor = vec4(mapped, 1.0);

    //FragColor = texture(screenTexture, TexCoords);
}