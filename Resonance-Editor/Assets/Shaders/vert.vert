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
    float3 normal : NORMAL;
    float2 tex : TEXCOORD;
    float4 tangent : TANGENT;
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

StructuredBuffer<Light> lights : register(t1);

PS_Input main(VS_Input input)
{
    PS_Input output;
    
    output.position = mul(viewProj, mul(model, float4(input.position, 1.0f)));
    output.color = input.color;
    output.normal = mul(transposeInverseModel, float4(input.normal, 1.0f));
    output.tex = input.texcoord;
    output.tangent = normalize(mul(model, input.tangent));
    output.fragPosition = mul(model, float4(input.position, 1.0)).xyz;
    output.fragViewPos = float3(inverseView[0][3], inverseView[1][3], inverseView[2][3]);
    output.fragLightSpacePos =mul(lights[0].mainViewProj, float4(output.fragPosition, 1.0f));
    return output;
}