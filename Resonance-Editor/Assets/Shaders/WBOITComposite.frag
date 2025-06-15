Texture2D opaqueColor : register(t0);
Texture2D accumTex : register(t1);
Texture2D revealTex : register(t2);

cbuffer FrameInfo : register(b3)
{
    float2 framebufferSize;
}

#define EPSILON 0.00001

bool isApproximatelyEqual(float a, float b)
{
    return abs(a-b) <= (abs(a) < abs(b) ? abs(b) : abs(a)) * EPSILON;
}

float max3(float3 v)
{
    return max(max(v.x, v.y), v.z);
}

float4 main(float4 pos : SV_Position) : SV_Target
{
    float2 uv = pos.xy;
    //uv = uv * 0.5 + 0.5; // Convert from NDC to UV space
    
    float4 opaque = opaqueColor.Load(int3(uv, 0));
    
    float revealage = revealTex.Load(int3(uv, 0)).r;
    
    if(isApproximatelyEqual(revealage, 1.0))
        discard;
    
    float4 accumulation = accumTex.Load(int3(uv, 0));
    
    //return accumulation;
    
    if(isinf(max3(abs(accumulation.rgb))))
        accumulation.rgb = float3(accumulation.a.xxx);
    
    float3 average_color = accumulation.rgb / max(accumulation.a, EPSILON);
    float3 finalColor = lerp(opaque.rgb, average_color,revealage);
    
    return float4(finalColor, 1.0);
}