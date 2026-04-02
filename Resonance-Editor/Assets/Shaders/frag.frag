#include "textures.hlsl"



#define M_PI 3.141592653589793f
#define c_MinRoughness 0.04

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
#ifdef USE_SPECULAR_TEXTURE
Texture2D texture_specular : register(t7, space1);
SamplerState texture_sampler_specular : register(s7, space1);
Texture2D texture_specular_color : register(t8, space1);
SamplerState texture_sampler_specular_color : register(s8, space1);
#endif

static const int LightType_Point = 0;
static const int LightType_Directional = 1;
static const int LightType_Spot = 3;

float clampedDot(float3 x, float3 y)
{
    return clamp(dot(x, y), 0.0, 1.0);
}

float3 F_Schlick(float3 f0, float3 f90, float VdotH)
{
    return f0 + (f90 - f0) * pow(clamp(1.0 - VdotH, 0.0, 1.0), 5.0);
}

float F_Schlick(float f0, float f90, float VdotH)
{
    float x = clamp(1.0 - VdotH, 0.0, 1.0);
    float x2 = x * x;
    float x5 = x * x2 * x2;
    return f0 + (f90 - f0) * x5;
}

float F_Schlick(float f0, float VdotH)
{
    float f90 = 1.0; //clamp(50.0 * f0, 0.0, 1.0);
    return F_Schlick(f0, f90, VdotH);
}

float3 F_Schlick(float3 f0, float f90, float VdotH)
{
    float x = clamp(1.0 - VdotH, 0.0, 1.0);
    float x2 = x * x;
    float x5 = x * x2 * x2;
    return f0 + (f90 - f0) * x5;
}

float3 F_Schlick(float3 f0, float VdotH)
{
    float f90 = 1.0; //clamp(dot(f0, vec3(50.0 * 0.33)), 0.0, 1.0);
    return F_Schlick(f0, f90, VdotH);
}

// Smith Joint GGX
// Note: Vis = G / (4 * NdotL * NdotV)
// see Eric Heitz. 2014. Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs. Journal of Computer Graphics Techniques, 3
// see Real-Time Rendering. Page 331 to 336.
// see https://google.github.io/filament/Filament.md.html#materialsystem/specularbrdf/geometricshadowing(specularg)
float V_GGX(float NdotL, float NdotV, float alphaRoughness)
{
    float alphaRoughnessSq = alphaRoughness * alphaRoughness;

    float GGXV = NdotL * sqrt(NdotV * NdotV * (1.0 - alphaRoughnessSq) + alphaRoughnessSq);
    float GGXL = NdotV * sqrt(NdotL * NdotL * (1.0 - alphaRoughnessSq) + alphaRoughnessSq);

    float GGX = GGXV + GGXL;
    if (GGX > 0.0)
    {
        return 0.5 / GGX;
    }
    return 0.0;
}


// The following equation(s) model the distribution of microfacet normals across the area being drawn (aka D())
// Implementation from "Average Irregularity Representation of a Roughened Surface for Ray Reflection" by T. S. Trowbridge, and K. P. Reitz
// Follows the distribution function recommended in the SIGGRAPH 2013 course notes from EPIC Games [1], Equation 3.
float D_GGX(float NdotH, float alphaRoughness)
{
    float alphaRoughnessSq = alphaRoughness * alphaRoughness;
    float f = (NdotH * NdotH) * (alphaRoughnessSq - 1.0) + 1.0;
    return alphaRoughnessSq / (M_PI * f * f);
}

float3 BRDF_lambertian(float3 diffuseColor)
{
    // see https://seblagarde.wordpress.com/2012/01/08/pi-or-not-to-pi-in-game-lighting-equation/
    return (diffuseColor / M_PI);
}

float3 BRDF_specularGGX(float alphaRoughness, float NdotL, float NdotV, float NdotH)
{
    float Vis = V_GGX(NdotL, NdotV, alphaRoughness);
    float D = D_GGX(NdotH, alphaRoughness);

    return float3((Vis * D).xxx);
}

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
    float range;
    float outerCutoff;
    int type;
    float _padding;
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
Texture2D PointShadowMaps[50] : register(t4);
SamplerState PointShadowSampler : register(s4);

StructuredBuffer<Light> lights : register(t1);

cbuffer flatData : register(b1, space1)
{
    float4 u_BaseColorFactor;
    float u_Roughness;
    float u_Metallic;
    float normalScalar;
    int u_FlipNormalY; // 0 = no flip, 1 = flip Y normal
    float4 u_EmissiveFactor; // W = alpha cutoff
    float4 u_SpecularFactor;
    float preCompF0;
};

//#define USE_NORMAL_TEXTURE

float getRangeAttenuation(float range, float distance)
{
    if (range <= 0.0)
    {
        // negative range means unlimited
        return 1.0 / pow(distance, 2.0);
    }
    return max(min(1.0 - pow(distance / range, 4.0), 1.0), 0.0) / pow(distance, 2.0);
}

float getSpotAttenuation(float3 pointToLight, float3 spotDirection, float outerConeCos, float innerConeCos)
{
    float actualCos = dot(normalize(spotDirection), normalize(-pointToLight));
    if (actualCos > outerConeCos)
    {
        if (actualCos < innerConeCos)
        {
            float angularAttenuation = (actualCos - outerConeCos) / (innerConeCos - outerConeCos);
            return angularAttenuation * angularAttenuation;
        }
        return 1.0;
    }
    return 0.0;
}

float3 getLighIntensity(Light light, float3 pointToLight)
{
    float rangeAttenuation = 1.0;
    float spotAttenuation = 1.0;
    
    if (light.type != LightType_Directional)
    {
        rangeAttenuation = getRangeAttenuation(light.range, length(pointToLight));
    }
    if (light.type == LightType_Spot)
    {
        //spotAttenuation = //getSpotAttenuation(pointToLight, light.direction, light.outerConeCos, light.innerConeCos);
    }

    return rangeAttenuation * spotAttenuation * light.color.a * light.color.rgb;
}



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

struct NormalInfo
{
    float3 ng; // Geometry normal
    float3 t; // Geometry tangent
    float3 b; // Geometry bitangent
    float3 n; // Shading normal
    float3 ntex; // Normal from texture, scaling is accounted for.
#if DEBUG == DEBUG_TANGENT_W
    float tangentWSign; // W component of the tangent attribute, used to determine handedness of TBN matrix
#endif
};

struct MaterialInfo
{
    float ior;
    float perceptualRoughness; // roughness value, as authored by the model creator
    float3 f0_dielectric;

    float alphaRoughness; // roughness mapped to a more linear change in the roughness

    float fresnel_w;

    float3 f90; // reflectance color at grazing angle
    float3 f90_dielectric;
    float metallic;

    float3 baseColor;

    float sheenRoughnessFactor;
    float3 sheenColorFactor;

    float3 clearcoatF0;
    float3 clearcoatF90;
    float clearcoatFactor;
    float3 clearcoatNormal;
    float clearcoatRoughness;

    // KHR_materials_specular 
    float specularWeight; // product of specularFactor and specularTexture.a

    float transmissionFactor;

    float thickness;
    float3 attenuationColor;
    float attenuationDistance;

    // KHR_materials_iridescence
    float iridescenceFactor;
    float iridescenceIor;
    float iridescenceThickness;

    float diffuseTransmissionFactor;
    float3 diffuseTransmissionColorFactor;

    // KHR_materials_anisotropy
    float3 anisotropicT;
    float3 anisotropicB;
    float anisotropyStrength;

    // KHR_materials_dispersion
    float dispersion;
};

NormalInfo getNormalInfo(PS_Input input)
{
    float2 uv_dx = ddx(input.tex);
    float2 uv_dy = ddy(input.tex);

    if (length(uv_dx) <= 1e-2)
    {
        uv_dx = float2(1.0, 0.0);
    }

    if (length(uv_dy) <= 1e-2)
    {
        uv_dy = float2(0.0, 1.0);
    }

    //float3 t_ = (uv_dy.y * ddx(input.fragPosition) - uv_dx.y * ddy(input.fragPosition)) /
    //    (uv_dx.x * uv_dy.y - uv_dy.x * uv_dx.y);

    float3 n, t, b, ng;

    // Compute geometrical TBN:
    // Trivial TBN computation, present as vertex attribute.
    // Normalize eigenvectors as matrix is linearly interpolated.
    t = normalize(input.v_TBN[0]);
    b = normalize(input.v_TBN[1]);
    ng = normalize(input.v_TBN[2]);

    // Compute normals:
    NormalInfo info;
    info.ng = ng;
#ifdef USE_NORMAL_TEXTURE
    info.ntex = texture_normal.Sample(texture_sampler_normal, input.tex).rgb * 2.0 - 1.0;
    if (u_FlipNormalY != 0)
    {
        info.ntex.y = -info.ntex.y;
    }
    info.ntex.xy *= normalScalar;
    info.ntex = normalize(info.ntex);

    info.n = normalize(t * info.ntex.x + b * info.ntex.y + ng * info.ntex.z);
#else
    info.n = ng;
#endif
    info.t = t;
    info.b = b;

#if DEBUG == DEBUG_TANGENT_W
    // only run this if debug channel is enabled
	//info.tangentWSign = v_TangentWSign;
#endif
    return info;
}

float4 getBaseColor(PS_Input input)
{
    float4 baseColor = u_BaseColorFactor;

#if defined(USE_ALBEDO_TEXTURE)
    baseColor *= texture_albedo.Sample(texture_sampler_albedo, input.tex);
#endif

    return baseColor * input.color;
}

MaterialInfo getMetallicRoughnessInfo(MaterialInfo info, PS_Input input)
{
    info.metallic = u_Metallic;
    info.perceptualRoughness = u_Roughness;

#ifdef USE_METALLICROUGHNESS_TEXTURE
    // Roughness is stored in the 'g' channel, metallic is stored in the 'b' channel.
    // This layout intentionally reserves the 'r' channel for (optional) occlusion map data
    float4 mrSample = texture_roughness.Sample(texture_sampler_roughness, input.tex);
    info.perceptualRoughness *= mrSample.g;
    info.metallic *= mrSample.b;
#endif

    return info;
}

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

float4 main(PS_Input input, bool isFrontFacing : SV_IsFrontFace) : SV_TARGET
{
    float4 baseColor = getBaseColor(input);
    
    float3 color = float3(0, 0, 0);

    float3 v = normalize(input.fragViewPos - input.fragPosition);
    NormalInfo normalInfo = getNormalInfo(input);
    float3 n = isFrontFacing ? normalInfo.n : -normalInfo.n;
    float3 t = normalInfo.t;
    float3 b = normalInfo.b;
    
    
    //return float4(normalInfo.n.xyz, 1.0f);

    float NdotV = clampedDot(n, v);
    float TdotV = clampedDot(t, v);
    float BdotV = clampedDot(b, v);

    MaterialInfo materialInfo;
    materialInfo.baseColor = baseColor.rgb;
    
    // The default index of refraction of 1.5 yields a dielectric normal incidence reflectance of 0.04.
    materialInfo.ior = 1.5;
    materialInfo.f0_dielectric = float3(0.04.xxx);
    materialInfo.specularWeight = 1.0;
    
    materialInfo.f90 = float3(1.0.xxx);
    materialInfo.f90_dielectric = materialInfo.f90;

    materialInfo = getMetallicRoughnessInfo(materialInfo, input);
    
    materialInfo.perceptualRoughness = clamp(materialInfo.perceptualRoughness, 0.0, 1.0);
    materialInfo.metallic = clamp(materialInfo.metallic, 0.0, 1.0);

    //TODO: add in all the extensions
    
    // Roughness is authored as perceptual roughness; as is convention,
    // convert to material roughness by squaring the perceptual roughness.
    materialInfo.alphaRoughness = materialInfo.perceptualRoughness * materialInfo.perceptualRoughness;
    
    float3 f_specular_dielectric = float3(0.0.xxx);
    float3 f_specular_metal = float3(0.0.xxx);
    float3 f_diffuse = float3(0.0.xxx);
    float3 f_dielectric_brdf_ibl = float3(0.0.xxx);
    float3 f_metal_brdf_ibl = float3(0.0.xxx);
    float3 f_emissive = float3(0.0.xxx);
    float3 clearcoat_brdf = float3(0.0.xxx);
    float3 f_sheen = float3(0.0.xxx);
    float3 f_specular_transmission = float3(0.0.xxx);
    float3 f_diffuse_transmission = float3(0.0.xxx);

    float clearcoatFactor = 0.0;
    float3 clearcoatFresnel = float3(0.0.xxx);

    float albedoSheenScaling = 1.0;
    float diffuseTransmissionThickness = 1.0;
    float3 diffuseTransmissionIBL = float3(0.0.xxx);
    
    f_diffuse = float3(0.0.xxx);
    f_specular_dielectric = float3(0.0.xxx);
    f_specular_metal = float3(0.0.xxx);
    float3 f_dielectric_brdf = float3(0.0.xxx);
    float3 f_metal_brdf = float3(0.0.xxx);
    
    for (int i = 0; i < lightCount; i++)
    {
        Light light = lights[i];
        
        float3 pointToLight;
        if (light.position.w == 1.0)
        {
            pointToLight = -light.direction.rgb;
        }
        else
        {
            pointToLight = light.position.xyz - input.fragPosition;
        }

        float3 l = normalize(pointToLight); // Direction from surface point to light
        float3 h = normalize(l + v); // Direction of the vector between l and v, called halfway vector
        float NdotL = clampedDot(n, l);
        float NdotV = clampedDot(n, v);
        float NdotH = clampedDot(n, h);
        float LdotH = clampedDot(l, h);
        float VdotH = clampedDot(v, h);

        float3 dielectric_fresnel = F_Schlick(materialInfo.f0_dielectric * materialInfo.specularWeight, materialInfo.f90_dielectric, abs(VdotH));
        float3 metal_fresnel = F_Schlick(baseColor.rgb, float3(1.0.xxx), abs(VdotH));
        
        float3 lightIntensity = getLighIntensity(light, pointToLight);
        
        //return float4(lightIntensity, 1.0);
        
        float3 l_diffuse = lightIntensity * NdotL * BRDF_lambertian(baseColor.rgb);
        float3 l_specular_dielectric = float3(0.0.xxx);
        float3 l_specular_metal = float3(0.0.xxx);
        float3 l_dielectric_brdf = float3(0.0.xxx);
        float3 l_metal_brdf = float3(0.0.xxx);
        float3 l_clearcoat_brdf = float3(0.0.xxx);
        float3 l_sheen = float3(0.0.xxx);
        float l_albedoSheenScaling = 1.0;
        float3 l_diffuse_btdf = float3(0.0.xxx);
    
        float3 intensity = getLighIntensity(light, pointToLight);
        
        l_specular_metal = intensity * NdotL * BRDF_specularGGX(materialInfo.alphaRoughness, NdotL, NdotV, NdotH);
        l_specular_dielectric = l_specular_metal;
        
        l_metal_brdf = metal_fresnel * l_specular_metal;
        l_dielectric_brdf = lerp(l_diffuse, l_specular_dielectric, dielectric_fresnel);
        
        float3 l_color = lerp(l_dielectric_brdf, l_metal_brdf, materialInfo.metallic);
        l_color = l_sheen + l_color * l_albedoSheenScaling;
        l_color = lerp(l_color, l_clearcoat_brdf, clearcoatFactor * clearcoatFresnel);
        color += l_color;
    }
    
    f_emissive = u_EmissiveFactor;
    
#ifdef USE_EMISSIVE_TEXTURE
    f_emissive *= texture_emissive.Sample(texture_sampler_emissive, input.tex).rgb;
#endif
    
    color = f_emissive * (1.0 - clearcoatFactor * clearcoatFresnel) + color;
    
    
#ifdef ALPHA_CUTOFF
    if(baseColor.a < u_EmissiveFactor.w)
        discard;
    baseColor.a = 1.0;
#endif
    
    
    return float4(color, baseColor.a);
}