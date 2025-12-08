#include "ShaderProperty.h"
#include "ShaderNode.h"

namespace REON::SG {
	ShaderValue ShaderProperty::GetDefaultValue()
	{
		switch (type)
		{
		case REON::SG::ShaderValueType::Undefined:
			REON_ERROR("ShaderValueType is Undefined, cannot get default value.");
			return ShaderValue();
			break;
		case REON::SG::ShaderValueType::Float:
			return 0.0f;
			break;
		case REON::SG::ShaderValueType::Float2:
			return glm::vec2(0.0f, 0.0f);
			break;
		case REON::SG::ShaderValueType::Float3:
			return glm::vec3(0.0f, 0.0f, 0.0f);
			break;
		case REON::SG::ShaderValueType::Float4:
			return glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
			break;
		case REON::SG::ShaderValueType::Float4x4:
			return glm::mat4(1.0f);
			break;
		case REON::SG::ShaderValueType::Bool:
			return false;
			break;
		case REON::SG::ShaderValueType::Texture2D:
			return "";
			break;
		case REON::SG::ShaderValueType::SamplerState:
			return SamplerData();
			break;
		default:
			REON_ERROR("Invalid ShaderValueType for default value: {0}", static_cast<int>(type));
			return ShaderValue();
			break;
		}
	}

	std::string ShaderProperty::GetValueAsString()
	{
		return std::visit([&](auto& v) -> std::string {
			using T = std::decay_t<decltype(v)>;

			if constexpr (std::is_same_v<T, float>) {
				return std::to_string(v) + "f";
			}
			else if constexpr (std::is_same_v<T, glm::vec2>) {
				return "float2(" + std::to_string(v.x) + "f, " + std::to_string(v.y) + "f)";
			}
			else if constexpr (std::is_same_v<T, glm::vec3>) {
				return "float3(" + std::to_string(v.x) + "f, " + std::to_string(v.y) + "f, " + std::to_string(v.z) + "f)";
			}
			else if constexpr (std::is_same_v<T, glm::vec4>) {
				return "float4(" + std::to_string(v.x) + "f, " + std::to_string(v.y) + "f, " + std::to_string(v.z) + "f, " + std::to_string(v.w) + "f)";
			}
			else if constexpr (std::is_same_v<T, bool>) {
				return v ? "true" : "false";
			}
			else if constexpr (std::is_same_v<T, std::string>) {
				return "\"" + v + "\"";
			}
			else {
				return "<unsupported>";
			}
			}, value);
	}
}