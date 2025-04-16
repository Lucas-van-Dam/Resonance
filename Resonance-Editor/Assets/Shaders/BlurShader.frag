#version 460 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D brightPassTexture;
uniform vec2 offset; // (1, 0) for horizontal, (0, 1) for vertical

void main() {
    const float weight[7] = float[](
        0.153388, 0.122649, 0.091552, 0.062883, 0.040437, 0.02501, 0.01463
    );

    vec2 tex_offset = offset / textureSize(brightPassTexture, 0);

    vec3 result = texture(brightPassTexture, TexCoords).rgb * weight[0];

    for(int i = 1; i < 7; ++i){
        result += texture(brightPassTexture, TexCoords + tex_offset * float(i)).rgb * weight[i];
        result += texture(brightPassTexture, TexCoords - tex_offset * float(i)).rgb * weight[i];
    }

    FragColor = vec4(result, 1.0);
}