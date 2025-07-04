#include "ShaderNodeLibrary.h"
#include <nlohmann/json.hpp>

namespace REON::SG {
	void ShaderNodeLibrary::RegisterTemplate(const ShaderNodeTemplate& nodeTemplate)
	{
		if (m_Templates.find(nodeTemplate.name) != m_Templates.end()) {
			REON_WARN("ShaderNodeLibrary: Template for type {0} already exists, overwriting.", nodeTemplate.name);
		}
		m_Templates[nodeTemplate.name] = nodeTemplate;
	}

	const ShaderNodeTemplate* ShaderNodeLibrary::GetTemplate(const std::string& type) const
	{
		if (m_Templates.find(type) != m_Templates.end()) {
			return &m_Templates.at(type);
		}
		else {
			REON_WARN("ShaderNodeLibrary: No template found for type: {0}", type);
			return nullptr;
		}
	}

	void ShaderNodeLibrary::Initialize(std::filesystem::path templateFile)
	{
		ShaderNodeTemplate propertyTemplate;
		propertyTemplate.name = "Property";
		propertyTemplate.codeTemplate = "{type} {Value} = {value};"; // TODO
		propertyTemplate.outputs.push_back({ "Value", ShaderValueType::Undefined, "", PinKind::Output });
		ShaderNodeVariant propertyVariant;
		propertyVariant.outputs.push_back({ "Value", ShaderValueType::Float, "", PinKind::Output });
		propertyTemplate.variants.push_back(propertyVariant);
		propertyVariant.outputs.clear();
		propertyVariant.outputs.push_back({ "Value", ShaderValueType::Float2, "", PinKind::Output });
		propertyTemplate.variants.push_back(propertyVariant);
		propertyVariant.outputs.clear();
		propertyVariant.outputs.push_back({ "Value", ShaderValueType::Float3, "", PinKind::Output });
		propertyTemplate.variants.push_back(propertyVariant);
		propertyVariant.outputs.clear();
		propertyVariant.outputs.push_back({ "Value", ShaderValueType::Float4, "", PinKind::Output });
		propertyTemplate.variants.push_back(propertyVariant);
		propertyVariant.outputs.clear();
		propertyVariant.outputs.push_back({ "Value", ShaderValueType::Float4x4, "", PinKind::Output });
		propertyTemplate.variants.push_back(propertyVariant);
		propertyVariant.outputs.clear();
		propertyVariant.outputs.push_back({ "Value", ShaderValueType::Bool, "", PinKind::Output });
		propertyTemplate.variants.push_back(propertyVariant);
		propertyVariant.outputs.clear();
		propertyVariant.outputs.push_back({ "Value", ShaderValueType::SamplerState, "", PinKind::Output });
		propertyTemplate.variants.push_back(propertyVariant);
		propertyVariant.outputs.clear();
		propertyVariant.outputs.push_back({ "Value", ShaderValueType::Texture2D, "", PinKind::Output });
		propertyTemplate.variants.push_back(propertyVariant);

		RegisterTemplate(propertyTemplate);


		if (!std::filesystem::exists(templateFile)) {
			REON_ERROR("ShaderNodeLibrary: Template file does not exist at path: {0}", templateFile.string());
			return;
		}
		std::ifstream templateStream(templateFile);
		if (templateStream.is_open()) {
			nlohmann::json jsonTemplates;
			templateStream >> jsonTemplates;
			for (const auto& item : jsonTemplates["nodes"]) {
				ShaderNodeTemplate nodeTemplate;
				if (item.contains("name")) {
					auto name = item["name"].get<std::string>();
					nodeTemplate.name = name;
				}
				if (item.contains("codeTemplate")) {
					nodeTemplate.codeTemplate = item["codeTemplate"].get<std::string>();
				}
				if (item.contains("inputs")) {
					for (const auto& input : item["inputs"]) {
						ShaderPinTemplate pin;
						pin.name = input["name"].get<std::string>();
						if (input.contains("type"))
							pin.type = ShaderValueTypeFromString(input["type"].get<std::string>());
						else
							pin.type = ShaderValueType::Undefined;
						pin.kind = PinKind::Input;
						if (input.contains("defaultValue")) {
							pin.defaultValue = input["defaultValue"].get<std::string>();
						}
						nodeTemplate.inputs.push_back(pin);
					}
				}
				if (item.contains("outputs")) {
					for (const auto& output : item["outputs"]) {
						ShaderPinTemplate pin;
						pin.name = output["name"].get<std::string>();
						if(output.contains("type"))
							pin.type = ShaderValueTypeFromString(output["type"].get<std::string>());
						else
							pin.type = ShaderValueType::Undefined;
						pin.kind = PinKind::Output;
						nodeTemplate.outputs.push_back(pin);
					}
				}
				if (item.contains("variants")) {
					for (const auto& variant : item["variants"]) {
						ShaderNodeVariant templateVariant;
						if (!variant.is_object()) {
							for (auto input : nodeTemplate.inputs) {
								if (input.type == ShaderValueType::Undefined)
									input.type = ShaderValueTypeFromString(variant.get<std::string>());
								templateVariant.inputs.push_back(input);
							}
							for (auto output : nodeTemplate.outputs) {
								if (output.type == ShaderValueType::Undefined)
									output.type = ShaderValueTypeFromString(variant.get<std::string>());
								templateVariant.outputs.push_back(output);
							}
						}

						nodeTemplate.variants.push_back(templateVariant);
					}
				}

				RegisterTemplate(nodeTemplate);
			}
		}
	}
	inline ShaderValueType ShaderNodeLibrary::ShaderValueTypeFromString(const std::string& str)
	{
		static const std::unordered_map<std::string, ShaderValueType> map = {
			{ "float", ShaderValueType::Float },
			{ "float2", ShaderValueType::Float2 },
			{ "float3", ShaderValueType::Float3 },
			{ "float4", ShaderValueType::Float4 },
			{ "bool", ShaderValueType::Bool },
			{ "SamplerState", ShaderValueType::SamplerState },
			{ "Texture2D", ShaderValueType::Texture2D },
			{ "float4x4", ShaderValueType::Float4x4 },
		};

		auto it = map.find(str);
		if (it != map.end()) {
			return it->second;
		} else {
			REON_WARN("ShaderNodeLibrary: Unknown shader value type: {0}", str);
			return ShaderValueType::Float;
		}
	}
}