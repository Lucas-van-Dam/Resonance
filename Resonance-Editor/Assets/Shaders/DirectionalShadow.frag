//#version 460 core

//#define BIAS 0.01

//void main() {
//    gl_FragDepth = gl_FragCoord.z;
//    gl_FragDepth += gl_FrontFacing ? BIAS : 0.0;
//}

struct PS_Output
{
    float depth : SV_Depth;
};

PS_Output main(float4 pos : SV_Position, bool isFrontFacing : SV_IsFrontFace)
{
    PS_Output output;
    output.depth = pos.z;
    if (isFrontFacing)
        output.depth += 0.01; // BIAS
    return output;
}