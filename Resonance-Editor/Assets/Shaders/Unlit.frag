struct PS_Input
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 tex : TEXCOORD;
};

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

#ifdef USE_ALBEDO_TEXTURE
Texture2D texture_albedo : register(t3, space1);
SamplerState texture_sampler_albedo : register(s3, space1);
#endif
#ifdef USE_EMISSIVE_TEXTURE
Texture2D texture_emissive : register(t6, space1);
SamplerState texture_sampler_emissive : register(s6, space1);
#endif

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

float4 main(PS_Input input) : SV_TARGET
{
#ifdef USE_ALBEDO_TEXTURE
    float4 baseColor = SRGBtoLINEAR(texture_albedo.Sample(texture_sampler_albedo, input.tex)) * u_BaseColorFactor; // TODO: look into maybe srgb to linear color
#else
    float4 baseColor = u_BaseColorFactor;
#endif
    
#ifdef USE_EMISSIVE_TEXTURE
    baseColor.rgb += SRGBtoLINEAR(texture_emissive.Sample(texture_sampler_emissive, input.tex)).rgb * u_EmissiveFactor.rgb;
#else
    baseColor.rgb += u_EmissiveFactor.rgb;
#endif
    
    return baseColor;
}