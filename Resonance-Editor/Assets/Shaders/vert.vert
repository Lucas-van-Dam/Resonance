struct VS_Input
{
    float3 position : POSITION;
    float4 color : COLOR;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD;
    float4 tangent : TANGENT;
    uint4 joints_0 : JOINTS_0;
    uint4 joints_1 : JOINTS_1;
    float4 weights_0 : WEIGHTS_0;
    float4 weights_1 : WEIGHTS_1;
};

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

cbuffer ObjectBuffer : register(b2, space2)
{
    float4x4 model;
    float4x4 transposeInverseModel;
    int paletteOffset;
    int jointCount;
};

cbuffer GlobalBuffer : register(b0)
{
    float4x4 viewProj;
    float4x4 inverseView;
    int lightCount;
    float3 _padding;
};

StructuredBuffer<Light> lights : register(t1);

//StructuredBuffer<float4x4> boneMatrices : register(t3);

PS_Input main(VS_Input input)
{
    PS_Input output;

    float4 localPos = float4(input.position, 1.0f);
    float3 localN = input.normal;
    float4 localT4 = float4(input.tangent.xyz, 0.0f); // direction
    float tanSign = input.tangent.w;


//    if (paletteOffset > 0)
//    {
    
//        uint4 j = input.joints_0; // JOINTS_0
//        float4 w = input.weights_0; // WEIGHTS_0

//// position (w=1)
//        float3 skinnedPos =
//      mul(boneMatrices[j.x], localPos) * w.x
//    + mul(boneMatrices[j.y], localPos) * w.y
//    + mul(boneMatrices[j.z], localPos) * w.z
//    + mul(boneMatrices[j.w], localPos) * w.w;

//// normal/tangent as directions (w=0)
//        float4 n4 = float4(localN, 0.0f);
//        float3 skinnedN4 =
//      mul(boneMatrices[j.x], n4) * w.x
//    + mul(boneMatrices[j.y], n4) * w.y
//    + mul(boneMatrices[j.z], n4) * w.z
//    + mul(boneMatrices[j.w], n4) * w.w;

//        float3 skinnedT4 =
//      mul(boneMatrices[j.x], localT4) * w.x
//    + mul(boneMatrices[j.y], localT4) * w.y
//    + mul(boneMatrices[j.z], localT4) * w.z
//    + mul(boneMatrices[j.w], localT4) * w.w;

//        localPos = float4(skinnedPos, 1.0f);
//        localN = normalize(skinnedN4.xyz);
//        localT4 = float4(normalize(skinnedT4.xyz), 0.0f);
        
//    }

// World position from (possibly skinned) local position
    float4 worldPos4 = mul(model, localPos);
    output.position = mul(viewProj, worldPos4);
    output.fragPosition = worldPos4.xyz;

// Attributes passthrough
    output.color = input.color;
    output.tex = input.texcoord;

// World-space normal/tangent
    output.normal = normalize(mul(transposeInverseModel, float4(localN, 0.0f)).xyz);

    float3 tangent = normalize(mul((float3x3) model, localT4.xyz));

// Orthonormalize tangent against normal
    tangent = normalize(tangent - dot(tangent, output.normal) * output.normal);

    float3 bitangent = cross(output.normal, tangent) * tanSign;
    output.tbn = float3x3(tangent, bitangent, output.normal);

// Camera pos
    output.fragViewPos = float3(inverseView[0][3], inverseView[1][3], inverseView[2][3]);

// Light space
    output.fragLightSpacePos = mul(lights[0].mainViewProj, float4(output.fragPosition, 1.0f));

    return output;
}