#include "ShaderGraphCodegen.h"

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <sstream>
#include <vector>
#include <iostream>
#include "ShaderNode.h"

namespace REON::SG {

	struct CodeGenContext {
		std::unordered_map<const ShaderNode*, std::string> nodeResultNames;
		std::unordered_set<const ShaderNode*> visited;
		std::stringstream code;
		int tempIndex = 0;


		std::string getNextVarName() {
			return "var" + std::to_string(tempIndex++);
		}
	};

	std::string resolveInput(const ShaderPin& pin, CodeGenContext& ctx);

	void resolveNodeVariant(ShaderNode* node)
	{
		if (!node || node->templateData->variants.empty()) return;

		for (auto& variant : node->templateData->variants) {
			bool match = true;

			for (size_t i = 0; i < variant.inputs.size(); ++i) {
				if (i >= node->inputs.size()) {
					match = false;
					break;
				}

				const auto& expectedType = variant.inputs[i].type;
				const auto& connectedPin = node->inputs[i];

				// Skip if not connected — could use override/defaults
				if (!connectedPin.node) continue;

				const ShaderNode* sourceNode = connectedPin.node;
				if (!sourceNode->activeVariant || sourceNode->activeVariant->outputs.empty()) {
					match = false;
					break;
				}

				const ShaderValueType actualType = sourceNode->activeVariant->outputs[0].type;
				if (!IsImplicitlyConvertible(actualType, expectedType)) {
					match = false;
					break;
				}
			}

			if (match) {
				node->activeVariant = &variant;
				return;
			}
		}

		// fallback: use first variant or set null
		node->activeVariant = &node->templateData->variants[0];
	}

	void emitNodeCode(const ShaderNode* node, CodeGenContext& ctx) {
		if (!node || ctx.visited.count(node)) return;
		ctx.visited.insert(node);


		const auto& tmpl = *node->templateData;
		std::unordered_map<std::string, std::string> replacements;


		// Determine type for variant nodes (simplified)
		std::string selectedType = "float"; // TODO: determine dynamically from connections
		replacements["type"] = selectedType;


		// Resolve inputs
		for (const auto& pin : node->inputs) {
			std::string resolved = resolveInput(pin, ctx);
			if (pin.templateData) {
				replacements[pin.templateData->name] = resolved;
			}
		}


		// Generate variable names for outputs
		for (const auto& pin : node->outputs) {
			std::string varName = ctx.getNextVarName();
			ctx.nodeResultNames[node] = varName;
			if (pin.templateData) {
				replacements[pin.templateData->name] = varName;
			}
			replacements["result"] = varName; // Fallback
		}

		if (node->type == ShaderNodeType::Property) {
			replacements["value"] = node->property->GetValueAsString();
		}

		// Fill in the code template
		std::string code = tmpl.codeTemplate;
		for (const auto& [key, value] : replacements) {
			std::string token = "{" + key + "}";
			size_t pos = 0;
			while ((pos = code.find(token, pos)) != std::string::npos) {
				code.replace(pos, token.length(), value);
			}
		}


		ctx.code << code << "\n";
	}


	std::string resolveInput(const ShaderPin& pin, CodeGenContext& ctx) {
		if (!pin.node->inputConnections.contains(pin.templateData->name) || !pin.templateData) return "0.0"; // fallback

		auto& connection = pin.node->inputConnections[pin.templateData->name];

		const ShaderNode* sourceNode = connection.node;
		emitNodeCode(sourceNode, ctx);

		auto it = ctx.nodeResultNames.find(sourceNode);
		if (it != ctx.nodeResultNames.end()) {
			return it->second;
		}


		return "0.0";
	}


	// Entry point for shader graph codegen
	std::string generateShaderCode(const ShaderNode* outputNode) {
		CodeGenContext ctx;
		emitNodeCode(outputNode, ctx);
		return ctx.code.str();
	}

}