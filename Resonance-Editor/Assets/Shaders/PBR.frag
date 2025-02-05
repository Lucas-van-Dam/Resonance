#version 460 core

#define PI 3.1415926535897932384626433832795

layout(binding = 0) uniform sampler2D texture_albedo;
layout(binding = 1) uniform sampler2D texture_normal;
layout(binding = 2) uniform sampler2D texture_roughness;
layout(binding = 3) uniform sampler2D texture_metallic;
layout(binding = 4) uniform sampler2D shadowMap;
layout(binding = 5) uniform samplerCube skyboxCube;
layout(binding = 6) uniform samplerCube irradianceMap;
layout(binding = 7) uniform samplerCube preFilterMap;
layout(binding = 8) uniform sampler2D brdfLUT;
layout(binding = 9) uniform samplerCube shadowCubes[10];



out vec4 FragColor;

in vec2 TexCoords;
in vec3 fragPosition;
in vec3 fragViewPos;
in vec4 fragLightSpacePos;
in vec3 fragNormal;

in vec3 T;
in vec3 Normal;

struct Light{
    //W = type
    vec4 position;
    //W = cutOff
    vec4 direction;
    //W = intensity
    vec4 lightColor;
    //directional light only
    mat4 projection;
    mat4 view;
};

layout(std140, binding = 1) buffer LightingBuffer {
    Light lights[];
};

uniform bool useAlbedoTexture = true;
uniform bool useRoughnessTexture = true;
uniform bool useMetallicTexture = true;
uniform bool useNormalTexture = true;

uniform vec4 albedo;
uniform float roughness = -1;
uniform float metallic = -1;

float roughnessInternal;
float metallicInternal;



uniform float far_plane;

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    FragColor = vec4(NdotV, 0,0,1.0);

    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

const vec3 poissonDisk[4] = {
vec3(0.355, 0.355, 0.0),
vec3(- 0.355, 0.355, 0.0),
vec3(0.355, - 0.355, 0.0),
vec3(- 0.355, - 0.355, 0.0)
};

float FindBlockerDistanceGrid(vec3 fragPos, Light light, int index, int gridResolution, float diskRadius) {
    vec3 fragToLight = fragPos - light.position.xyz;
    float currentDepth = length(fragToLight);
    float avgBlockerDepth = 0.0;
    int blockerCount = 0;

    // Grid-based sampling for the blocker search
    for (int x = -gridResolution; x <= gridResolution; ++x) {
        for (int y = -gridResolution; y <= gridResolution; ++y) {
            vec3 offset = vec3(float(x), float(y), 0.0) * (diskRadius / float(gridResolution));
            float closestDepth = texture(shadowCubes[index], fragToLight + offset).r * far_plane;

            if (closestDepth < currentDepth) {
                avgBlockerDepth += closestDepth;
                blockerCount++;
            }
        }
    }

    if (blockerCount > 0) {
        avgBlockerDepth /= float(blockerCount);
    }

    return avgBlockerDepth;
}

float ComputePenumbraSize(float lightToFragmentDist, float avgBlockerDepth, float lightRadius) {
    return (lightToFragmentDist - avgBlockerDepth) / avgBlockerDepth * lightRadius;
}

float PCSSShadowCalculationGrid(vec3 normal, vec3 fragPos, Light light, int index) {
    vec3 fragToLight = fragPos - light.position.xyz;
    float currentDepth = length(fragToLight);
    float bias = max((0.05 + 0.005 * currentDepth) * (1.0 - abs(dot(normalize(normal), normalize(fragToLight)))), 0.005);
    
    // Blocker Search
    int blockerGridRes = 4; // Adjust grid resolution for accuracy
    float diskRadius = 0.05;
    float avgBlockerDepth = FindBlockerDistanceGrid(fragPos, light, index, blockerGridRes, diskRadius);

    if (avgBlockerDepth == 0.0) {
        return 1.0; // No blockers found, fully lit
    }

    // Calculate Penumbra Size
    float lightToFragmentDist = currentDepth;
    float lightRadius = 0.3; // Light radius for softness control
    float penumbraSize = ComputePenumbraSize(lightToFragmentDist, avgBlockerDepth, lightRadius);

    // Shadow calculation with PCF and variable radius
    float shadow = 0.0;
    int pcfGridRes = 4; // PCF grid resolution for smoother shadows

    for (int x = -pcfGridRes; x <= pcfGridRes; ++x) {
        for (int y = -pcfGridRes; y <= pcfGridRes; ++y) {
            vec3 offset = vec3(float(x), float(y), 0.0) * (penumbraSize / float(pcfGridRes));
            float closestDepth = texture(shadowCubes[index], fragToLight + offset).r * far_plane;

            shadow += currentDepth - bias > closestDepth ? 1.0 : 0.0;
        }
    }

    // Normalize by the total number of samples
    int totalSamples = (2 * pcfGridRes + 1) * (2 * pcfGridRes + 1);
    shadow /= float(totalSamples);

    return shadow;
}


float AdditionalShadowCalculation(vec3 normal, vec3 fragPos, Light light, int index) {
    // get vector between fragment position and light position
    vec3 fragToLight = fragPos - light.position.xyz;
    // use the light to fragment vector to sample from the depth map
    float currentDepth = length(fragToLight);
     float bias = max((0.005 + 0.0005 * currentDepth) * (1.0 - abs(dot(normalize(normal), normalize(fragToLight)))), 0.0005);
    //float angleFactor = 1.0 - abs(dot(normalize(normal), normalize(-fragToLight)));
    //float bias = max((0.0005 * exp(5.0 * angleFactor)) * (1.0 - angleFactor), 0.00005);
    float shadow = 0.0;
    int samples = 4; // Number of samples for PCF
    float diskRadius = 0.05; // Radius of the sampling disk

    for (int i = 0; i < samples; ++i) {
        // Apply the predefined offsets within the disk radius
        vec3 offset = diskRadius * poissonDisk[i];
        float closestDepth = texture(shadowCubes[index], fragToLight + offset).r;
        closestDepth *= far_plane; // Re-transform back to original value

        // Accumulate shadow value
        shadow += currentDepth - bias > closestDepth ? 1.0 : 0.0;
    }

    // Average the shadow value
    shadow /= float(samples);

    return shadow;
}

float MainShadowCalculation(vec4 fragPosLightSpace)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords.xyz = projCoords.xyz * 0.5 + 0.5;
    float currentDepth = projCoords.z;
    float bias = max(0.005 * (1.0 - abs(dot(normalize(Normal), normalize(lights[0].direction.xyz)))), 0.0005);

    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

    if(projCoords.z > 1.0)
    shadow = 0.0;

    return shadow;
}

void main()
{
    vec3 V    = normalize(fragViewPos - fragPosition);
    vec3 N = normalize(Normal);
    vec3 R = reflect(-V, N);

    vec4 diffuseColor = useAlbedoTexture ? texture(texture_albedo, TexCoords).xyzw : albedo;

    roughnessInternal = useRoughnessTexture ? texture(texture_roughness, TexCoords).g : roughness;
    //roughnessInternal = useMetallicTexture ? texture(texture_metallic, TexCoords).g : roughness;
    metallicInternal = useRoughnessTexture ? texture(texture_roughness, TexCoords).b : metallic;

    if(useNormalTexture){
        vec3 fragTangent = normalize(T);
        fragTangent = normalize(fragTangent - dot(fragTangent, N) * N);
        vec3 fragBiTangent = cross(N, fragTangent);

        mat3 TBN = mat3(fragTangent, fragBiTangent, N);
        N = texture(texture_normal, TexCoords).rgb;
        N = normalize(N * 2.0 - 1.0);
        N = normalize((TBN) * N);
    }

    // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, diffuseColor.xyz, metallicInternal);

    // reflectance equation
    vec3 Lo = vec3(0.0);
    int pointLightCount = 0;
    for(int i = 0; i < lights.length(); i++)
    {
        vec3 L;
        vec3 radiance;
        float shadow = 0.0;

        if (lights[i].position.w == 1.0) // Directional light
        {
            L = normalize(-lights[i].direction.xyz);
            radiance = lights[i].lightColor.xyz; // No attenuation for directional lights
            shadow = MainShadowCalculation(fragLightSpacePos);
        }
        else // Point light
        {
            L = normalize(lights[i].position.xyz - fragPosition);
            float distance = length(lights[i].position.xyz - fragPosition);
            float attenuation = 1.0 / (distance * distance);
            radiance = lights[i].lightColor.xyz * attenuation;
            shadow = PCSSShadowCalculationGrid(N, fragPosition, lights[i], pointLightCount++);

            // FragColor = vec4(shadow,0,0,1);
            // return;
        }

        vec3 H = normalize(V + L);
        float distance = length(lights[i].position.xyz - fragPosition);
        float attenuation = 1.0 / (distance * distance);

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughnessInternal);
        float G   = GeometrySmith(N, V, L, roughnessInternal);
        vec3 F    = fresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);

        

                
        vec3 numerator    = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
        vec3 specular = numerator / denominator;

        //FragColor = vec4(specular, 1);

        // kS is equal to Fresnel
        vec3 kS = F;
        // for energy conservation, the diffuse and specular light can't
        // be above 1.0 (unless the surface emits light); to preserve this
        // relationship the diffuse component (kD) should equal 1.0 - kS.
        vec3 kD = vec3(1.0) - kS;
        // multiply kD by the inverse metalness such that only non-metals
        // have diffuse lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
        kD *= 1.0 - metallicInternal;

        // scale light by NdotL
        float NdotL = max(dot(N, L), 0.0);

        // add to outgoing radiance Lo
        Lo += (kD * diffuseColor.xyz / PI + specular) * radiance * NdotL * (1.0 - shadow);  // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
    }

    vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughnessInternal);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallicInternal;

    vec3 irradiance = texture(irradianceMap, N).rgb;
    vec3 diffuse      = irradiance * diffuseColor.rgb;

    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = textureLod(preFilterMap, R,  roughnessInternal * MAX_REFLECTION_LOD).rgb;
    vec2 brdf  = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughnessInternal)).rg;
    vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

    vec3 ambient = (kD * diffuse + specular);

    vec3 color = Lo + ambient;

    // HDR tonemapping
    //color = color / (color + vec3(1.0));

    // gamma correct
    //color = pow(color, vec3(1.0/2.2));

    FragColor = vec4(color, 1.0);
}