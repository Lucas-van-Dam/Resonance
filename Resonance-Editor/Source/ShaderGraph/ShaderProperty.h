#pragma once

#include <variant>
#include <string>
#include <glm/glm.hpp>
#include <REON/Rendering/Structs/Texture.h>
#include <unordered_set>

namespace REON::SG
{
	using ShaderValue = std::variant<float, glm::vec2, glm::vec3, glm::vec4, glm::mat4, bool, std::string, SamplerData>;

	struct ShaderPropertyUI {
		
	};

	enum class ShaderValueType;

	struct ShaderNode;

	struct ShaderProperty
	{
		std::string name;
		ShaderValueType type;
		ShaderValue value;
		bool visible = true;
		std::unordered_set<ShaderNode*> referencingNodes;

		ShaderValue GetDefaultValue();
		std::string GetValueAsString();
	};

}