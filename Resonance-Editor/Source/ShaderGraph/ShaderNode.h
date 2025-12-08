#pragma once

#include <string>
#include <Reon.h>
#include <unordered_map>
#include <memory>
#include <imgui/imgui.h>
#include <variant>
#include <REON/Rendering/Structs/Texture.h>
#include "ShaderProperty.h"

namespace REON::SG
{
	using ShaderValue = std::variant<float, glm::vec2, glm::vec3, glm::vec4, glm::mat4, bool, std::string, SamplerData>;

	enum class ShaderValueType {
		Undefined = -1,
		Float = 0,
		Float2,
		Float3,
		Float4,
		Float4x4,
		Bool,
		Texture2D,
		SamplerState
	};

	enum class PinKind
	{
		Output,
		Input
	};

	enum class ShaderNodeType
	{
		Undefined = -1,
		Standard,
		Property,
		Master,
	};

	struct ShaderPinTemplate {
		std::string name;
		ShaderValueType type;
		std::string defaultValue;
		PinKind kind;
	};
	
	struct ShaderNode;

	struct ShaderPin {
		ShaderNode* node = nullptr; // Pointer to the node this pin belongs to
		const ShaderPinTemplate* templateData = nullptr; // Pointer to the template data for this pin
		ShaderValue overrideValue; // Value override for this pin (used when not connected)
	};

	struct ShaderNodeVariant {
		std::vector<ShaderPinTemplate> inputs;
		std::vector<ShaderPinTemplate> outputs;
	};

	struct ShaderNodeTemplate : public Object
	{
		std::string name; 
		std::string codeTemplate;
		std::vector<ShaderPinTemplate> inputs;
		std::vector<ShaderPinTemplate> outputs;
		std::vector<ShaderNodeVariant> variants;

		static std::string ShaderValueTypeToHLSL(ShaderValueType type) {
			switch (type) {
			case ShaderValueType::Float: return "float";
			case ShaderValueType::Float2:  return "float2";
			case ShaderValueType::Float3:  return "float3";
			case ShaderValueType::Float4:  return "float4";
			case ShaderValueType::Bool:  return "bool";
			case ShaderValueType::SamplerState: return "SamplerState";
			case ShaderValueType::Texture2D: return "Texture2D";
			case ShaderValueType::Float4x4:  return "float4x4";
			default: return "unknown";
			}
		}
		// TODO: Maybe add a system for one json structure per node, with overloads being stored in a map or something
	};

	struct ShaderNode : public Object {
		ShaderNodeType type = ShaderNodeType::Undefined; // Type of the node
		std::string name;
		ImVec4 color = ImVec4(0.0, 255.0, 255.0, 255.0);
		const ShaderNodeTemplate* templateData; // Template data for this node
		std::unordered_map<std::string, ShaderPin> inputConnections; // pin name -> (node id, node output name)
		std::unordered_map<std::string, std::vector<ShaderPin>> outputConnections; // pin name -> (node id, node input name)
		std::vector<ShaderPin> inputs;
		std::vector<ShaderPin> outputs;
		glm::vec2 position; // Position in the editor

		const ShaderNodeVariant* activeVariant = nullptr;

		std::shared_ptr<ShaderProperty> property = nullptr; // Pointer to the property this node represents (only when type is PropertyNode)


		bool isAnyOutputConnected() const {
			return !outputConnections.empty();
		}
		
		ShaderNode() : templateData(nullptr), position(0.0f, 0.0f) {}
		ShaderNode(const ShaderNode&) = default;
		ShaderNode& operator=(const ShaderNode&) = default;

		ShaderNode(ShaderNode&&) = default;
		ShaderNode& operator=(ShaderNode&&) = default;
	};

    // Returns true if 'from' can be implicitly converted to 'to'
    bool IsImplicitlyConvertible(ShaderValueType from, ShaderValueType to);

	ShaderValueType ShaderValueTypeFromString(const std::string& str);
}