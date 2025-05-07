struct VS_Input
{
    float2 position : POSITION;
    float3 color : COLOR;
};

struct PS_Input
{
    float4 position : SV_POSITION;
    float3 color : COLOR;
};

cbuffer UniformBufferObject : register(b0)
{
    float4x4 model;
    float4x4 view;
    float4x4 projection;
};

PS_Input main(VS_Input input)
{
    PS_Input output;
    
    output.position = mul(projection, mul(view, mul(model, float4(input.position, 0.0f, 1.0f))));
    output.color = input.color;
    
    return output;
}