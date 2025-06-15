struct VS_Input
{
    float3 position : POSITION;
    float4 color : COLOR;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD;
    float4 tangent : TANGENT;
};

struct PS_Input
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 tex : TEXCOORD;
};

cbuffer ObjectBuffer : register(b2, space2)
{
    float4x4 model;
    float4x4 transposeInverseModel;
};

cbuffer GlobalBuffer : register(b0)
{
    float4x4 viewProj;
    float4x4 inverseView;
    int lightCount;
    float3 _padding;
};

PS_Input main(VS_Input input)
{
    PS_Input output;
    
    output.position = mul(viewProj, mul(model, float4(input.position, 1.0f)));
    output.color = input.color;
    output.tex = input.texcoord;
    return output;
}