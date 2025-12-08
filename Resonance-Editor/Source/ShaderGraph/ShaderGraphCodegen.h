#pragma once

#include <string>

namespace REON::SG {

    struct ShaderNode; // Forward declare

    // Main entry point: generate HLSL code from a graph starting at output node
    std::string generateShaderCode(const ShaderNode* outputNode);

}