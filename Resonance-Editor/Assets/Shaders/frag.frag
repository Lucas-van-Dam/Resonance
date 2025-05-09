struct PS_Input
{
    float4 pos : SV_POSITION;
    float3 color : COLOR;
    float2 tex : TEXCOORD0;
};

Texture2D tex : register(t1);
SamplerState texSampler : register(s1);

float4 main(PS_Input input) : SV_TARGET
{
    return float4(tex.Sample(texSampler, input.tex * 2).rgb, 1.0);
}