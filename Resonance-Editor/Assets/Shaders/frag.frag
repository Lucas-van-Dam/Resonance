#define PI 3.1415926535897932384626433832795

struct PS_Input
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float3 normal : NORMAL;
    float2 tex : TEXCOORD;
    float3x3 tbn : TBN;
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
    float4 albedo;
    float roughness;
    float metallic;
    int useAlbedoTexture;
    int useNormalTexture;
    int useMetallicRoughnessTexture;
    float normalScalar;
};

Texture2D texture_albedo : register(t3, space1);
SamplerState texture_sampler_albedo : register(s3, space1);
Texture2D texture_normal : register(t4, space1);
SamplerState texture_sampler_normal : register(s4, space1);
Texture2D texture_roughness : register(t5, space1);
SamplerState texture_sampler_roughness : register(s5, space1);

float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    
    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

float3 fresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float3 fresnelSchlickRoughness(float cosTheta, float3 F0, float roughness)
{
    return F0 + (max(1.0 - roughness, F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float MainShadowCalculation(float4 fragPosLightSpace, float3 normal)
{
    float3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords.xy = projCoords.xy * 0.5 + 0.5;
    //projCoords.z = projCoords.z * 0.5 + 0.5;

    float currentDepth = projCoords.z;
    float bias = max(0.005 * (1.0 - abs(dot(normalize(normal), normalize(lights[0].direction.xyz)))), 0.0005);
    
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
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

    return shadow;
}

float4 main(PS_Input input, bool isFrontFacing : SV_IsFrontFace) : SV_TARGET
{
    float3 V = normalize(input.fragViewPos - input.fragPosition);
    
    float3 N = normalize(input.normal);
    
    //return float4(N, 1.0);
    
    if (useNormalTexture)
    {
        N = texture_normal.Sample(texture_sampler_normal, input.tex).rgb;
        
        N.g = 1.0 - N.g;
        
        N = normalize(N * 2.0 - 1.0);
        
        //return float4(N, 1.0);
        
        
        N = normalize(mul(N, input.tbn));
        
        if(!isFrontFacing)
            N = -N;
        
        return float4(N, 1.0);
        
    }
    
        //return float4(N, 1.0);
    
    
    //return float4(N, 1.0f);
    
    //float3 R = reflect(-V, N);
    
    
    
    //return float4(input.fragViewPos, 1.0);
    
    float4 diffuseColor = useAlbedoTexture ? texture_albedo.Sample(texture_sampler_albedo, input.tex) : albedo;
    float roughnessInternal = useRoughnessTexture ? texture_roughness.Sample(texture_sampler_roughness, input.tex).g : roughness;
    float metallicInternal = useMetallicTexture ? texture_roughness.Sample(texture_sampler_roughness, input.tex).b : metallic;
    //return float4(metallicInternal, metallicInternal, metallicInternal, 1.0f);
    // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)
    float3 F0 = lerp(0.04.xxx, diffuseColor.xyz, metallicInternal);
    
    //return float4(roughnessInternal, roughnessInternal, roughnessInternal, 1.0);
    
    float3 Lo = 0.0;
    int pointLightCount = 0;
    
    for (int i = 0; i < lightCount; i++)
    {
        float3 L;
        float3 radiance;
        float shadow;
            
        if (lights[i].position.w == 1.0)
        {
            L = normalize(lights[i].direction.xyz);
            radiance = lights[i].color.xyz;
            shadow = MainShadowCalculation(input.fragLightSpacePos, N);
        }
        else
        {
            L = normalize(lights[i].position.xyz - input.fragPosition);
            float distance = length(lights[i].position.xyz - input.fragPosition);
            float attenuation = 1.0 / (distance * distance);
            radiance = lights[i].color.xyz * attenuation;
        }
        
        float3 H = normalize(V + L);
        
        float NDF = DistributionGGX(N, H, roughnessInternal);
        float G = GeometrySmith(N, V, L, roughnessInternal);
        float3 F = fresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);
        
        float3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
        float3 specular = numerator / max(denominator, 0.001);
        
        if (dot(H, V) < 0.0)
            return float4(0.0, 1.0, 1.0, 1.0);

        
        // kS is equal to Fresnel
        float3 kS = F;
        // for energy conservation, the diffuse and specular light can't
        // be above 1.0 (unless the surface emits light); to preserve this
        // relationship the diffuse component (kD) should equal 1.0 - kS.
        float3 kD = 1.0 - kS;
        // multiply kD by the inverse metalness such that only non-metals
        // have diffuse lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
        kD *= 1.0 - metallicInternal;
        
        //return float4(specular, 1.0);
        
        float NdotL = max(dot(N, L), 0.0);

        Lo += (kD * diffuseColor.xyz / PI + specular) * radiance * NdotL * (1.0 - shadow);
    }

    // ambient lighting
    float3 ambient = 0.05 * diffuseColor.xyz;
    
    return float4(Lo, 1.0);
}