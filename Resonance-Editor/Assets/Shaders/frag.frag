#define M_PI 3.141592653589793f
#define c_MinRoughness 0.04

struct PS_Input
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float3 normal : NORMAL;
    float2 tex : TEXCOORD;
    float3x3 v_TBN : TBN;
    float3 fragPosition : FRAG_POSITION;
    float3 fragViewPos : FRAG_VIEW_POS;
    float4 fragLightSpacePos : FRAG_LIGHT_SPACE_POS;
};

struct Light
{
    float4 position;
    float4 direction;
    float4 color;
    float4x4 mainViewProj;
};

cbuffer GlobalBuffer : register(b0)
{
    column_major float4x4 viewProj;
    column_major float4x4 inverseView;
    int lightCount;
    float3 _padding;
};

Texture2D DirectionalShadowMap : register(t2);
SamplerState DirectionalShadowSampler : register(s2);
Texture2D PointShadowMaps[50] : register(t3);
SamplerState PointShadowSampler : register(s3);

StructuredBuffer<Light> lights : register(t1);

cbuffer flatData : register(b1, space1)
{
    float4 u_BaseColorFactor;
    float u_Roughness;
    float u_Metallic;
    float normalScalar;
    int u_FlipNormalY; // 0 = no flip, 1 = flip Y normal
    float4 u_EmissiveFactor; // W = alpha cutoff
};

//#define USE_NORMAL_TEXTURE

#ifdef USE_ALBEDO_TEXTURE
Texture2D texture_albedo : register(t3, space1);
SamplerState texture_sampler_albedo : register(s3, space1);
#endif
#ifdef USE_NORMAL_TEXTURE
Texture2D texture_normal : register(t4, space1);
SamplerState texture_sampler_normal : register(s4, space1);
#endif
#ifdef USE_METALLICROUGHNESS_TEXTURE
Texture2D texture_roughness : register(t5, space1);
SamplerState texture_sampler_roughness : register(s5, space1);
#endif
#ifdef USE_EMISSIVE_TEXTURE
Texture2D texture_emissive : register(t6, space1);
SamplerState texture_sampler_emissive : register(s6, space1);
#endif

struct PBRInfo
{
    float NdotL; // cos angle between normal and light direction
    float NdotV; // cos angle between normal and view direction
    float NdotH; // cos angle between normal and half vector
    float LdotH; // cos angle between light direction and half vector
    float VdotH; // cos angle between view direction and half vector
    float perceptualRoughness; // roughness value, as authored by the model creator (input to shader)
    float metalness; // metallic value at the surface
    float3 reflectance0; // full reflectance color (normal incidence angle)
    float3 reflectance90; // reflectance color at grazing angle
    float alphaRoughness; // roughness mapped to a more linear change in the roughness (proposed by [2])
    float3 diffuseColor; // color contribution from diffuse lighting
    float3 specularColor; // color contribution from specular lighting
};

float MainShadowCalculation(float4 fragPosLightSpace, float3 normal)
{
    float3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords.xy = projCoords.xy * 0.5 + 0.5;
    //projCoords.z = projCoords.z * 0.5 + 0.5;

    float currentDepth = projCoords.z;
    float bias = max(0.005 * (1.0 - abs(dot(normalize(normal), normalize(lights[0].direction.xyz)))), 0.0005);
    
    //return bias;
    //PCF
    float shadow = 0.0;
    int sizex, sizey;
    DirectionalShadowMap.GetDimensions(sizex, sizey);
    float2 texelSize = 1.0 / float2(sizex, sizey);
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float pcfDepth = DirectionalShadowMap.Sample(DirectionalShadowSampler, projCoords.xy + float2(x, y) * texelSize).r;
            //return pcfDepth;
            shadow += currentDepth > pcfDepth + bias ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

    return shadow;
}

float4 SRGBtoLINEAR(float4 srgbIn)
{
#ifdef SRGB_FAST_APPROXIMATION
    float3 linOut = pow(srgbIn.xyz,2.2.xxx);
#else //SRGB_FAST_APPROXIMATION
    float3 bLess = step(0.04045.xxx, srgbIn.xyz);
    float3 linOut = lerp(srgbIn.xyz / 12.92.xxx, pow((srgbIn.xyz + 0.055.xxx) / 1.055.xxx, 2.4.xxx), bLess);
#endif //SRGB_FAST_APPROXIMATION
    return float4(linOut, srgbIn.w);;
}

//diffuse_brdf
float3 diffuse(PBRInfo pbrInputs)
{
    return pbrInputs.diffuseColor / M_PI;
}

//conductor_fresnel
float3 specularReflection(PBRInfo pbrInputs)
{
    return pbrInputs.reflectance0 + (pbrInputs.reflectance90 - pbrInputs.reflectance0) * pow(clamp(1.0 - pbrInputs.VdotH, 0.0, 1.0), 5.0);
}

float geometricOcclusion(PBRInfo pbrInputs)
{
    float NdotL = pbrInputs.NdotL;
    float NdotV = pbrInputs.NdotV;
    float r = pbrInputs.alphaRoughness;

    float attenuationL = 2.0 * NdotL / (NdotL + sqrt(r * r + (1.0 - r * r) * (NdotL * NdotL)));
    float attenuationV = 2.0 * NdotV / (NdotV + sqrt(r * r + (1.0 - r * r) * (NdotV * NdotV)));
    return attenuationL * attenuationV;
}

// specular_brdf
float microfacetDistribution(PBRInfo pbrInputs)
{
    float roughnessSq = pbrInputs.alphaRoughness * pbrInputs.alphaRoughness;
    float f = (pbrInputs.NdotH * roughnessSq - pbrInputs.NdotH) * pbrInputs.NdotH + 1.0;
    return roughnessSq / (M_PI * f * f);
}

float3 getNormal(float3x3 inTbn, float2 texCoord)
{
    float3x3 tbn = inTbn;
    
#ifdef USE_NORMAL_TEXTURE
    float3 n = texture_normal.Sample(texture_sampler_normal, texCoord);
    n.g = lerp(n.g, 1.0 - n.g, u_FlipNormalY);
    n = normalize(mul(float3(normalScalar, normalScalar, 1.0) * (2.0 * n - 1.0), tbn));
    //return float3(1.0, 0.0, 1.0);
#else
    float3 n = normalize(float3(tbn[0][2], tbn[1][2], tbn[2][2]));
#endif
    
    return n;
}

float4 main(PS_Input input, bool isFrontFacing : SV_IsFrontFace) : SV_TARGET
{
    float perceptualRoughness = u_Roughness;
    float metallic = u_Metallic;
    
#ifdef USE_METALLICROUGHNESS_TEXTURE
        float4 sample = texture_roughness.Sample(texture_sampler_roughness, input.tex);
        perceptualRoughness = sample.g * perceptualRoughness;
        metallic = sample.b * metallic;
        //return float4(1.0, 0.0, 1.0, 1.0);
#endif
    
    perceptualRoughness = clamp(perceptualRoughness, c_MinRoughness, 1.0);
    metallic = clamp(metallic, 0.0, 1.0);
    
    float alphaRoughness = perceptualRoughness * perceptualRoughness;

#ifdef USE_ALBEDO_TEXTURE
    float4 baseColor = SRGBtoLINEAR(texture_albedo.Sample(texture_sampler_albedo, input.tex)) * u_BaseColorFactor; // TODO: look into maybe srgb to linear color
#else
    float4 baseColor = u_BaseColorFactor;
#endif
    
#ifdef ALPHA_CUTOFF
    if(baseColor.a < u_EmissiveFactor.w)
        discard;
#endif
    
    float3 f0 = 0.04.xxx;
    float3 diffuseColor = baseColor.rgb * (1.xxx - f0);
    diffuseColor *= 1.0 - metallic;
    float3 specularColor = lerp(f0, baseColor.rgb, metallic);
    float3 clampedSpecular = saturate(specularColor);
    
    float reflectance = max(max(specularColor.r, specularColor.g), specularColor.b);
    
    float reflectance90 = clamp(reflectance * 25.0, 0.0, 1.0);
    float3 specularEnvironmentR0 = specularColor.rgb;
    float3 specularEnvironmentR90 = 1.0.xxx * reflectance90;
    
    //return float4(reflectance90, 0.0, 0.0, 1.0);
    
    float3 n = getNormal(input.v_TBN, input.tex);
    
    //return float4(n, 1.0);
    float3 v = normalize(input.fragViewPos - input.fragPosition);
    
    float3 color;
    
    for (int i = 0; i < lightCount; i++)
    {
        float3 l;
        float3 radiance;
        float shadow = 0.0;
            
        if (lights[i].position.w == 1.0)
        {
            l = normalize(-lights[i].direction.xyz);
            radiance = lights[i].color.xyz;
            shadow = MainShadowCalculation(input.fragLightSpacePos, n);
            //return float4(shadow, 0.0, 0.0, 1.0);
        }
        else
        {
            l = normalize(lights[i].position.xyz - input.fragPosition);
            float distance = length(lights[i].position.xyz - input.fragPosition);
            float attenuation = 1.0 / (distance * distance);
            radiance = lights[i].color.xyz * attenuation;
        }
        
        float3 h = normalize(l + v);
        
        float NdotL = clamp(dot(n, l), 0.001, 1.0);
        float NdotV = clamp(abs(dot(n, v)), 0.001, 1.0);
        float NdotH = clamp(dot(n, h), 0.0, 1.0);
        float LdotH = clamp(dot(l, h), 0.0, 1.0);
        float VdotH = clamp(dot(v, h), 0.0, 1.0);
        
        PBRInfo pbrInputs;
        pbrInputs.NdotL = NdotL;
        pbrInputs.NdotV = NdotV;
        pbrInputs.NdotH = NdotH;
        pbrInputs.LdotH = LdotH;
        pbrInputs.VdotH = VdotH;
        pbrInputs.perceptualRoughness = perceptualRoughness;
        pbrInputs.metalness = metallic;
        pbrInputs.reflectance0 = specularEnvironmentR0;
        pbrInputs.reflectance90 = specularEnvironmentR90;
        pbrInputs.alphaRoughness = alphaRoughness;
        pbrInputs.diffuseColor = diffuseColor;
        pbrInputs.specularColor = specularColor;
        
        float3 F = specularReflection(pbrInputs);
        float G = geometricOcclusion(pbrInputs);
        float D = microfacetDistribution(pbrInputs);
        
        //return float4(NdotL, 0.0, 0.0, 1.0);
        //return float4(F, 1.0);
        float3 diffuseContrib = (1.0 - F) * diffuse(pbrInputs);
        float3 specularContrib = F * D * G / (4.0 * NdotL * NdotV);
        //return float4(radiance, 1.0);
        color = (diffuseContrib + specularContrib) * radiance * NdotL * (1.0 - shadow);
    }
    
#ifdef USE_EMISSIVE_TEXTURE
    color += SRGBtoLINEAR(texture_emissive.Sample(texture_sampler_emissive, input.tex)).rgb * u_EmissiveFactor.rgb;
#endif
    
    return float4(color, 1.0);
}