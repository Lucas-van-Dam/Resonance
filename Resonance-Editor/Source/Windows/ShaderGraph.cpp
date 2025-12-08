#include "ShaderGraph.h"
#include <ShaderGraph/ShaderNodeLibrary.h>
#include <imgui/misc/cpp/imgui_stdlib.h>
#include <glm/gtc/type_ptr.hpp>
#include <ShaderGraph/ShaderGraphCodegen.h>

namespace REON::EDITOR {

	using namespace ax::Drawing;
	using namespace ax::Widgets;
	std::unordered_map<std::string, const SG::ShaderPin*> ShaderGraph::pins;

	bool ShaderGraph::createNewNode = false;
	const SG::ShaderPin* ShaderGraph::newNodeLinkPin = nullptr;
	const SG::ShaderPin* ShaderGraph::newLinkPin = nullptr;
	SG::ShaderNode* ShaderGraph::m_MasterNode = nullptr;

	void ShaderGraph::initialize()
	{
		ed::Config config;
		config.SettingsFile = "ShaderGraph.json";
		m_Context = ed::CreateEditor(&config);

		//ed::SetCurrentEditor(m_Context);
		//ed::Style& style = ed::GetStyle();
		//style.NodePadding = ImVec4(8, 8, 8, 8);
		//style.PinRadius = 5.0f;  // Make pin circle bigger
		//ed::SetCurrentEditor(nullptr);

		ed::SetCurrentEditor(m_Context);
		CreateNode("Master");
		m_MasterNode = &nodes.back(); // COULD BECOME PROBLEM LATER (RACE CONDITION)
		m_MasterNode->type = SG::ShaderNodeType::Master;
		ed::SetNodePosition(MakeNodeId(m_MasterNode->GetID()), ImVec2{ 0,0 });
		ed::CenterNodeOnScreen(MakeNodeId(m_MasterNode->GetID()));
		ed::SetCurrentEditor(nullptr);
	}

	void ShaderGraph::shutdown()
	{
		if (m_Context)
		{
			ed::DestroyEditor(m_Context);
			m_Context = nullptr;
		}
	}

	ImColor ShaderGraph::GetIconColor(ShaderValueType type)
	{
		switch (type)
		{
		default:
		case ShaderValueType::Bool:			return ImColor(169, 6, 245);
		//case ShaderValueType::Int:			return ImColor(68, 201, 156);
		case ShaderValueType::Float:		return ImColor(6, 229, 245);
		case ShaderValueType::Float2:		return ImColor(6, 245, 43);
		case ShaderValueType::Float3:		return ImColor(237, 245, 6);
		case ShaderValueType::Float4:		return ImColor(245, 6, 237);
		case ShaderValueType::Float4x4:		return ImColor(6, 45, 245);
		case ShaderValueType::Texture2D:	return ImColor(245, 6, 10);
		case ShaderValueType::SamplerState:	return ImColor(84, 84, 84);
		}
	};

	void ShaderGraph::DrawPinIcon(const ShaderPin& pin, bool connected, int alpha)
	{
		IconType iconType;
		ImColor  color = GetIconColor(pin.templateData->type);
		color.Value.w = alpha / 255.0f;
		switch (pin.templateData->type)
		{
		case ShaderValueType::Bool:     iconType = IconType::Circle; break;
		case ShaderValueType::Float:    iconType = IconType::Circle; break;
		default:
			iconType = IconType::Circle;
		}

		ax::Widgets::Icon(ImVec2(static_cast<float>(24), static_cast<float>(24)), iconType, connected, color, ImColor(32, 32, 32, alpha));
	};

	bool ShaderGraph::CanCreateLink(const SG::ShaderPin* a, const SG::ShaderPin* b)
	{
		if (!a || !b || a == b || a->templateData->kind == b->templateData->kind || a->node == b->node)
			return false;

		const ShaderNodeVariant* variant;
		bool typeChangable = false;
		if (a->templateData->kind == PinKind::Input)
			typeChangable = CanChangeType(*a, b->templateData->type, variant);
		else
			typeChangable = CanChangeType(*b, a->templateData->type, variant);

		if (!typeChangable)
			return false;

		return true;
	}

	const ShaderPin* ShaderGraph::findPin(ed::PinId id) {
		std::string guid = idToGuidMap[id.Get()];
		std::string nodeGuid = guid.substr(0, guid.find_first_of('_'));
		size_t firstUnderScore = guid.find('_');
		std::string pinName = guid.substr(firstUnderScore + 1, guid.find('_', firstUnderScore + 1) - firstUnderScore - 1);
		for (auto& node : nodes) {
			if (node.GetID() == nodeGuid) {
				for (auto& pin : node.inputs) {
					if (pin.templateData->name == pinName) {
						return &pin;
					}
				}
				for (auto& pin : node.outputs) {
					if (pin.templateData->name == pinName) {
						return &pin;
					}
				}
			}
		}
		return nullptr;
	}

	bool ShaderGraph::findLink(ed::PinId id, ShaderGraphLink& linkRef)
	{
		for (auto& link : links) {
			if (link.endPin == id || link.startPin == id) {
				linkRef = link;
				return true;
			}
		}
		return false;
	}

	bool ShaderGraph::findLink(ed::LinkId id, ShaderGraphLink& linkRef)
	{
		for (auto& link : links) {
			if (MakeLinkId(link.GetID()) == id) {
				linkRef = link;
				return true;
			}
		}
		return false;
	}

	bool ShaderGraph::IsPinLinked(ed::PinId id)
	{
		if (!id)
			return false;

		for (auto& link : links)
			if (link.startPin == id || link.endPin == id)
				return true;

		return false;
	}

	void ShaderGraph::render()
	{
		ed::SetCurrentEditor(m_Context);
		if (ImGui::Begin("Shader Graph Window")) {
			ImVec2 windowPos = ImGui::GetWindowPos();
			ImGui::SetNextWindowSize(ImVec2(250, 300), ImGuiCond_FirstUseEver);
			ImGui::SetNextWindowPos(windowPos + ImVec2(300, 10), ImGuiCond_FirstUseEver);

			ImGuiWindowFlags flags =
				ImGuiWindowFlags_NoCollapse |
				ImGuiWindowFlags_NoSavedSettings |
				ImGuiWindowFlags_NoDocking;

			if (ImGui::Begin("Node Settings", nullptr, flags)) {
				if (selectedNode.Get() != 0) {

				}
				if (selectedProperty != -1) {
					auto& prop = properties[selectedProperty];

					ImGui::PushID(static_cast<int>(selectedProperty));

					ImGui::InputText("Name", &prop->name);

					const char* typeNames[] = { "float", "float2", "float3", "float4", "float4x4", "bool", "texture2D", "sampler2D" };
					int typeIndex = static_cast<int>(prop->type);
					if (ImGui::Combo("Type", &typeIndex, typeNames, IM_ARRAYSIZE(typeNames))) {
						prop->type = static_cast<ShaderValueType>(typeIndex);
						prop->value = prop->GetDefaultValue();
						for (auto& node : prop->referencingNodes) {
							if (!node->outputConnections.empty()) {
								for (auto& connection : node->outputConnections["Value"]) {
									auto it = std::find_if(links.begin(), links.end(),
										[&](const ShaderGraphLink& link) {
											return link.startPin == MakePinId(node->GetID(), node->outputs[0].templateData->name, false)
												&& link.endPin == MakePinId(connection.node->GetID(), connection.templateData->name, true);
										});

									if (it != links.end()) {
										ed::DeleteLink(MakeLinkId(it->GetID()));
									}
									else {
										REON_WARN("ShaderGraph: Could not find link for property {0} in node {1}", prop->name, node->GetID());
									}
								}
							}
							for (auto& variant : node->templateData->variants) {
								if (variant.outputs[0].type == prop->type) {
									ChangeNodeVariant(node, variant);
									break;
								}
							}
						}
					}

					std::visit([&](auto& value) {
						using T = std::decay_t<decltype(value)>;

						if constexpr (std::is_same_v<T, float>) {
							ImGui::InputFloat("Value", &value);
						}
						else if constexpr (std::is_same_v<T, glm::vec2>) {
							ImGui::InputFloat2("Value", glm::value_ptr(value));
						}
						else if constexpr (std::is_same_v<T, glm::vec3>) {
							ImGui::InputFloat3("Value", glm::value_ptr(value));
						}
						else if constexpr (std::is_same_v<T, glm::vec4>) {
							ImGui::InputFloat4("Value", glm::value_ptr(value));
						}
						else if constexpr (std::is_same_v<T, bool>) {
							ImGui::Checkbox("Value", &value);
						}
						else if constexpr (std::is_same_v<T, std::string>) {
							char buffer[256];
							std::strncpy(buffer, value.c_str(), sizeof(buffer));
							if (ImGui::InputText("Value", buffer, sizeof(buffer))) {
								value = std::string(buffer); // Safe overwrite
							}
						}
						}, prop->value);

					//if (ImGui::Button("Delete")) {
					//	for (auto& node : prop->referencingNodes) {
					//		ed::DeleteNode(MakeNodeId(node->GetID()));
					//	}
					//	properties.erase(properties.begin() + selectedProperty);
					//	selectedProperty = -1;
					//}

					ImGui::Separator();
					ImGui::PopID();
				}
			}
			ImGui::End();

			ImGui::SetNextWindowSize(ImVec2(250, 300), ImGuiCond_FirstUseEver);
			ImGui::SetNextWindowPos(windowPos + ImVec2(0, 10), ImGuiCond_FirstUseEver);

			if (ImGui::Begin("Shader Properties", nullptr, flags)) {
				ImGui::Text("Properties");
				int i = 0;
				for (size_t i = 0; i < properties.size(); i++) {
					if (ImGui::Selectable((properties[i]->name + "##" + std::to_string(i)).c_str(), i == selectedProperty)) {
						selectedProperty = i;
					}

					if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
						ImGui::SetDragDropPayload("PROPERTY_DRAG", &properties[i], sizeof(std::shared_ptr<ShaderProperty>));
						ImGui::Text("%s", properties[i]->name.c_str());
						ImGui::EndDragDropSource();
					}

					if (ImGui::BeginPopupContextItem((properties[i]->name + "##Context" + std::to_string(i)).c_str())) {
						if (ImGui::MenuItem("Delete")) {
							for (auto& node : properties[i]->referencingNodes) {
								ed::DeleteNode(MakeNodeId(node->GetID()));
							}
							properties.erase(properties.begin() + i);
							if (selectedProperty == i) {
								selectedProperty = -1; // Reset selection if we deleted the selected property
							}
						}
						ImGui::EndPopup();
					}
				}

				if (ImGui::Button("Add Property")) {
					properties.push_back(std::make_shared<ShaderProperty>("NewProperty", ShaderValueType::Float, 0.0f));
				}
				if (ImGui::Button("Test generation")) {
					REON_INFO(::generateShaderCode(m_MasterNode));
				}
			}

			bool propertiesHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_RootAndChildWindows);

			ImGui::End();

			ed::Begin("Shader Graph");

			util::BlueprintNodeBuilder builder(0, 1, 1);

			for (auto& node : nodes) {
				DrawShaderNode(node, builder);
			}

			for (auto link : links) {
				ed::Link(MakeLinkId(link.GetID()),
					link.startPin,
					link.endPin,
					link.color,
					2.0f);
			}

			if (!createNewNode) {
				if (ed::BeginCreate(ImVec4(1, 1, 1, 1), 2.0f)) {
					auto showLabel = [](const char* label, ImColor color)
						{
							ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ImGui::GetTextLineHeight());
							auto size = ImGui::CalcTextSize(label);

							auto padding = ImGui::GetStyle().FramePadding;
							auto spacing = ImGui::GetStyle().ItemSpacing;

							ImGui::SetCursorPos(ImGui::GetCursorPos() + ImVec2(spacing.x, -spacing.y));

							auto rectMin = ImGui::GetCursorScreenPos() - padding;
							auto rectMax = ImGui::GetCursorScreenPos() + size + padding;

							auto drawList = ImGui::GetWindowDrawList();
							drawList->AddRectFilled(rectMin, rectMax, color, size.y * 0.15f);
							ImGui::TextUnformatted(label);
						};

					ed::PinId startPinId = 0, endPinId = 0;
					if (ed::QueryNewLink(&startPinId, &endPinId))
					{
						auto startPin = findPin(startPinId);
						auto endPin = findPin(endPinId);

						newLinkPin = startPin ? startPin : endPin;

						if (startPin->templateData->kind == PinKind::Input)
						{
							std::swap(startPin, endPin);
							std::swap(startPinId, endPinId);
						}

						if (startPin && endPin)
						{
							bool canConnect = false;
							bool variantFound = false;
							const ShaderNodeVariant* variant = nullptr;
							if (startPinId == endPinId)
							{
								ed::RejectNewItem(ImColor(255, 0, 0), 2.0f);
							}
							else if (endPin->templateData->kind == startPin->templateData->kind)
							{
								showLabel("x Incompatible Pin Kind", ImColor(45, 32, 32, 180));
								ed::RejectNewItem(ImColor(255, 0, 0), 2.0f);
							}
							else if (endPin->node == startPin->node)
							{
								showLabel("x Cannot connect to self", ImColor(45, 32, 32, 180));
								ed::RejectNewItem(ImColor(255, 0, 0), 1.0f);
							}
							else if (endPin->templateData->type != startPin->templateData->type)
							{
								if (CanChangeType(*endPin, startPin->templateData->type, variant)) {
									variantFound = true;
									canConnect = true;
								}
								if (!variantFound) {
									showLabel("x Incompatible Pin Type", ImColor(45, 32, 32, 180));
									ed::RejectNewItem(ImColor(255, 128, 128), 1.0f);
								}
							}
							else {
								canConnect = true;
							}

							if (canConnect)
							{
								bool replace = endPin->node->inputConnections.contains(endPin->templateData->name);
								if (replace)
									showLabel("+ Replace Link", ImColor(32, 45, 32, 180));
								else
									showLabel("+ Create Link", ImColor(32, 45, 32, 180));
								if (ed::AcceptNewItem(ImColor(128, 255, 128), 4.0f))
								{
									if (variantFound && variant)
										ChangeNodeVariant(endPin->node, *variant);
									if (replace) {
										ShaderGraphLink oldLink;
										if (findLink(endPinId, oldLink)) {
											std::erase(links, oldLink);
										}
									}
									ShaderGraphLink link;
									link.color = GetIconColor(startPin->templateData->type);
									link.startPin = startPinId;
									link.endPin = endPinId;
									links.emplace_back(link);
									links.back().color = GetIconColor(startPin->templateData->type);
									endPin->node->inputConnections[endPin->templateData->name] = *startPin;
									startPin->node->outputConnections[startPin->templateData->name].emplace_back(*endPin);
								}
							}
						}

						ed::PinId pinId = 0;
						if (ed::QueryNewNode(&pinId))
						{
							newLinkPin = findPin(pinId);
							if (newLinkPin)
								showLabel("+ Create Node", ImColor(32, 45, 32, 180));

							if (ed::AcceptNewItem())
							{
								createNewNode = true;
								newNodeLinkPin = findPin(pinId);
								newLinkPin = nullptr;
								ed::Suspend();
								ImGui::OpenPopup("Create New Node");
								ed::Resume();
							}
						}
					}
				}
				else
					newLinkPin = nullptr;

				ed::EndCreate();

				if (ed::BeginDelete()) {
					ed::NodeId nodeId;
					while (ed::QueryDeletedNode(&nodeId)) {
						if (ed::AcceptDeletedItem()) {
							auto it = std::find_if(nodes.begin(), nodes.end(), [&](const ShaderNode& n) { return n.GetID() == GetGuidFromId(nodeId.Get()); });
							if (it != nodes.end()) {
								if (it->property) {
									it->property->referencingNodes.erase(&(*it));
								}
								nodes.erase(it);
							}
						}
					}

					ed::LinkId linkId;
					while (ed::QueryDeletedLink(&linkId)) {
						if (ed::AcceptDeletedItem()) {
							ShaderGraphLink link;
							if (findLink(linkId, link)) {
								std::erase(links, link);
								if (auto startPin = findPin(link.startPin))
									startPin->node->outputConnections.erase(startPin->templateData->name);
								if (auto endPin = findPin(link.endPin))
									endPin->node->inputConnections.erase(endPin->templateData->name);
							}
						}
					}

				}
				ed::EndDelete();

			}

			static bool openPopup = false;
			if (!propertiesHovered && ed::ShowBackgroundContextMenu()) {
				openPopup = true;
			}
			ed::End();

			if (ImGui::BeginDragDropTarget()) {
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("PROPERTY_DRAG")) {
					IM_ASSERT(payload->DataSize == sizeof(std::shared_ptr<ShaderProperty>));
					std::shared_ptr<ShaderProperty> prop = *static_cast<std::shared_ptr<ShaderProperty>*>(payload->Data);

					if (prop) {
						CreateNode(prop);
					}
					else {
						REON_ERROR("ShaderGraph: Dragged property is null!");
					}
				}
				ImGui::EndDragDropTarget();
			}

			if (openPopup) {
				//ImGui::SetNextWindowPos(popupScreenPos);
				//ImGui::SetNextWindowFocus();
				ImGui::OpenPopup("NodeCreatePopup");
				openPopup = false;
			}

			if (ImGui::BeginPopup("NodeCreatePopup")) {
				auto& templates = SG::ShaderNodeLibrary::GetInstance().GetAllTemplates();
				for (auto& nodeTemplate : templates) {
					if (nodeTemplate.first == "Property") {
						continue; // Skip property nodes in the create menu
					}
					if (ImGui::MenuItem(nodeTemplate.first.c_str())) {
						CreateNode(nodeTemplate.first);
					}
				}
				ImGui::EndPopup();
			}
		}
		ImGui::End();
		ed::SetCurrentEditor(nullptr);
	}

	void ShaderGraph::DrawShaderNode(SG::ShaderNode& node, util::BlueprintNodeBuilder builder)
	{
		ed::NodeId nodeId = ed::NodeId(MakeNodeId(node.GetID()));

		builder.Begin(nodeId);

		if (node.type != ShaderNodeType::Property) {
			builder.Header(node.color);
			ImGui::Spring(0);
			ImGui::TextUnformatted(node.name.c_str());
			ImGui::Spring(1);
			ImGui::Dummy(ImVec2(0, 28));
			ImGui::Spring(0);
			builder.EndHeader();
		}

		for (const ShaderPin& input : node.inputs) {
			if (input.templateData->kind == PinKind::Output)
				continue;

			auto alpha = ImGui::GetStyle().Alpha;
			if (newLinkPin && !CanCreateLink(newLinkPin, &input) && &input != newLinkPin)
				alpha = alpha * (48.0f / 255.0f);

			ed::PinId pinId = MakePinId(node.GetID(), input.templateData->name, true);
			builder.Input(pinId);
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
			DrawPinIcon(input, IsPinLinked(pinId), (int)(alpha * 255));
			ImGui::Spring(0);
			if (!input.templateData->name.empty() && !input.node->inputConnections.contains(input.templateData->name))
			{
				ImGui::TextUnformatted(input.templateData->name.c_str());
				ImGui::Spring(0);
			}
			//if (input.templateData->type == ShaderValueType::Bool)
			//{
			//	bool a;
			//	ImGui::Checkbox("boolInput", &a);
			//	ImGui::Spring(0);
			//}
			ImGui::PopStyleVar();
			builder.EndInput();
		}

		if (node.type == ShaderNodeType::Property) {
			builder.Middle();

			ImGui::Spring(1, 0);
			ImGui::TextUnformatted(node.property->name.c_str());
			ImGui::Spring(1, 0);
		}

		for (auto& output : node.outputs)
		{

			ed::PinId pinId = MakePinId(node.GetID(), output.templateData->name, false);

			auto alpha = ImGui::GetStyle().Alpha;
			if (newLinkPin && !CanCreateLink(newLinkPin, &output) && &output != newLinkPin)
				alpha = alpha * (48.0f / 255.0f);

			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
			builder.Output(pinId);
			if (!output.templateData->name.empty() && node.outputs.size() > 1)
			{
				ImGui::Spring(0);
				ImGui::TextUnformatted(output.templateData->name.c_str());
			}
			ImGui::Spring(0);
			DrawPinIcon(output, IsPinLinked(pinId), (int)(alpha * 255));
			ImGui::PopStyleVar();
			builder.EndOutput();
		}

		builder.End();
	}

	void ShaderGraph::CreateNode(const std::string& templateName)
	{
		auto nodeTemplate = SG::ShaderNodeLibrary::GetInstance().GetTemplate(templateName);
		SG::ShaderNode tempNode;
		nodes.push_back(tempNode);
		ShaderNode& newNode = nodes.back();
		newNode.templateData = std::move(nodeTemplate);
		newNode.name = newNode.templateData->name;
		newNode.type = ShaderNodeType::Standard;
		const std::vector<ShaderPinTemplate>* inputs;
		const std::vector<ShaderPinTemplate>* outputs;
		if (!newNode.templateData->variants.empty()) {
			const ShaderNodeVariant& variant = newNode.templateData->variants.front();
			inputs = &variant.inputs;
			outputs = &variant.outputs;
		}
		else {
			inputs = &newNode.templateData->inputs;
			outputs = &newNode.templateData->outputs;
		}
		for (const ShaderPinTemplate& input : *inputs) {
			ShaderPin newInput;
			newInput.templateData = &input;
			newInput.node = &newNode;
			newInput.overrideValue = input.defaultValue;
			newNode.inputs.push_back(newInput);
			MakePinId(newNode.GetID(), input.name, true);
		}
		for (const ShaderPinTemplate& output : *outputs) {
			ShaderPin newOutput;
			newOutput.templateData = &output;
			newOutput.node = &newNode;
			newOutput.overrideValue = output.defaultValue;
			newNode.outputs.push_back(newOutput);
			MakePinId(newNode.GetID(), output.name, false);
		}

		ImVec2 canvasPos = ed::ScreenToCanvas(ImGui::GetMousePos());
		//ImVec2 popupScreenPos = ed::CanvasToScreen();


		ed::SetNodePosition(MakeNodeId(newNode.GetID()), canvasPos);
	}

	void ShaderGraph::CreateNode(std::shared_ptr<ShaderProperty> property)
	{
		auto nodeTemplate = SG::ShaderNodeLibrary::GetInstance().GetTemplate("Property");
		SG::ShaderNode tempNode;
		nodes.push_back(tempNode);
		ShaderNode& newNode = nodes.back();
		newNode.templateData = std::move(nodeTemplate);
		newNode.name = newNode.templateData->name;
		newNode.type = ShaderNodeType::Property;
		newNode.property = property;
		property->referencingNodes.insert(&newNode);
		const std::vector<ShaderPinTemplate>* inputs;
		const std::vector<ShaderPinTemplate>* outputs;
		if (!newNode.templateData->variants.empty()) {
			for (const auto& variant : newNode.templateData->variants) {
				if (variant.outputs.front().type == property->type) {
					inputs = &variant.inputs;
					outputs = &variant.outputs;
					break;
				}
			}
		}
		else {
			inputs = &newNode.templateData->inputs;
			outputs = &newNode.templateData->outputs;
		}
		for (const ShaderPinTemplate& input : *inputs) {
			ShaderPin newInput;
			newInput.templateData = &input;
			newInput.node = &newNode;
			newInput.overrideValue = input.defaultValue;
			newNode.inputs.push_back(newInput);
			MakePinId(newNode.GetID(), input.name, true);
		}
		for (const ShaderPinTemplate& output : *outputs) {
			ShaderPin newOutput;
			newOutput.templateData = &output;
			newOutput.node = &newNode;
			newOutput.overrideValue = output.defaultValue;
			newNode.outputs.push_back(newOutput);
			MakePinId(newNode.GetID(), output.name, false);
		}

		ImVec2 canvasPos = ed::ScreenToCanvas(ImGui::GetMousePos());
		//ImVec2 popupScreenPos = ed::CanvasToScreen();


		ed::SetNodePosition(MakeNodeId(newNode.GetID()), canvasPos);
	}

	void ShaderGraph::ChangeNodeVariant(SG::ShaderNode* node, const SG::ShaderNodeVariant& variant)
	{
		for (auto& pin : variant.inputs) {
			for (auto& nodePin : node->inputs) {
				if (nodePin.templateData->name == pin.name) {
					nodePin.templateData = &pin;
				}
			}
		}

		for (auto& pin : variant.outputs) {
			for (auto& nodePin : node->outputs) {
				if (nodePin.templateData->name == pin.name) {
					nodePin.templateData = &pin;
				}
			}
		}
	}

	bool ShaderGraph::CanChangeType(const SG::ShaderPin& pin, const ShaderValueType& type, const ShaderNodeVariant*& variant)
	{
		if (pin.templateData->type == type)
			return true;

		for (auto& variantOption : pin.node->templateData->variants) {
			bool variantAvailable = false;

			for (auto& input : variantOption.inputs) {
				if (input.type == type) {
					variant = &variantOption;
					variantAvailable = true;
					break;
				}
			}

			if (!variantAvailable) {
				for (auto& output : variantOption.outputs) {
					if (output.type == type) {
						variant = &variantOption;
						variantAvailable = true;
						break;
					}
				}
			}
			if (!variantAvailable)
			{
				for (auto& input : variantOption.inputs) {
					if (input.type == type || REON::SG::IsImplicitlyConvertible(type, input.type)) {
						variant = &variantOption;
						variantAvailable = true;
						break;
					}
				}

				if (!variantAvailable) {
					for (auto& output : variantOption.outputs) {
						if (output.type == type || REON::SG::IsImplicitlyConvertible(type, output.type)) {
							variant = &variantOption;
							variantAvailable = true;
							break;
						}
					}
				}
				if (!variantAvailable)
					continue;
			}

			if (pin.node->inputConnections.empty() || (pin.node->inputConnections.size() == 1 && pin.node->inputConnections.contains(pin.templateData->name)))
				return true;

			bool allInputsCompatible = true;
			for (const auto& connection : pin.node->inputConnections) {
				if (pin.templateData->name == connection.first)
					continue;
				bool foundCompatible = false;
				for (const auto& input : variant->inputs) {
					if (connection.second.templateData->type == input.type ||
						REON::SG::IsImplicitlyConvertible(connection.second.templateData->type, input.type)) {
						foundCompatible = true;
						break;
					}
				}
				if (!foundCompatible) {
					allInputsCompatible = false;
					break;
				}
			}
			if (!allInputsCompatible)
				continue;

			bool allOutputsCompatible = true;
			for (const auto& outputConnection : pin.node->outputConnections) {
				for (const auto& connectedPin : outputConnection.second) {
					bool foundCompatible = false;
					for (const auto& output : variant->outputs) {
						if (connectedPin.templateData->type == output.type ||
							REON::SG::IsImplicitlyConvertible(output.type, connectedPin.templateData->type)) {
							foundCompatible = true;
							break;
						}
					}
					if (!foundCompatible) {
						allOutputsCompatible = false;
						break;
					}
				}
				if (!allOutputsCompatible)
					break;
			}
			if (allOutputsCompatible)
				return true;
		}

		if (REON::SG::IsImplicitlyConvertible(pin.templateData->type, type)) {
			variant = nullptr;
			return true;
		}
		return false;
	}
}