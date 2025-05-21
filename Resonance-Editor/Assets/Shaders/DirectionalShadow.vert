struct VS_Input
{
    float3 Position : POSITION;
};

struct VS_Output
{
    float4 Position : SV_Position;
};

cbuffer ModelMatrix : register(b1, space1)
{
    matrix model;
};

cbuffer LightSpaceMatrix : register(b0)
{
    matrix lightSpaceMatrix;
};

VS_Output main(VS_Input input)
{
    VS_Output output;
    output.Position = mul(lightSpaceMatrix, mul(model, float4(input.Position, 1.0)));
    return output;
}