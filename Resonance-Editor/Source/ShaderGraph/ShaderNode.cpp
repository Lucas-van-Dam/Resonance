#include "ShaderNode.h"
#include <Windows/ShaderGraph.h>

namespace REON::SG {

bool IsImplicitlyConvertible(ShaderValueType from, ShaderValueType to)
{
    if (from == to)
        return true;

    // Float can be converted to Float2/3/4
    if (from == ShaderValueType::Float) {
        if (to == ShaderValueType::Float2 || to == ShaderValueType::Float3 || to == ShaderValueType::Float4)
            return true;
    }
	// Float2 can be converted to Float/Float3/4 (to float truncates the vector)
    if (from == ShaderValueType::Float2) {
        if (to == ShaderValueType::Float || to == ShaderValueType::Float3 || to == ShaderValueType::Float4)
            return true;
    }
	// Float3 can be converted to Float/Float2/4 (to float/2 truncates the vector)
    if (from == ShaderValueType::Float3) {
        if (to == ShaderValueType::Float || to == ShaderValueType::Float2 || to == ShaderValueType::Float4)
            return true;
    }
	// Float4 can be converted to Float2/3 (to float/2/3 truncates the vector)
    if (from == ShaderValueType::Float4) {
        if (to == ShaderValueType::Float || to == ShaderValueType::Float2 || to == ShaderValueType::Float3)
            return true;
    }
    return false;
}

ShaderValueType ShaderValueTypeFromString(const std::string& str) {
    if (str == "float") return ShaderValueType::Float;
    if (str == "float2") return ShaderValueType::Float2;
    if (str == "float3") return ShaderValueType::Float3;
    if (str == "float4") return ShaderValueType::Float4;
    // add more if needed
    return ShaderValueType::Undefined;
}

}