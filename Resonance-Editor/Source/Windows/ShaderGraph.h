#pragma once
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_node_editor.h>
#include <string>
#include <Reon.h>
#include <ShaderGraph/ShaderNode.h>
#include <utilities/drawing.h>
#include <utilities/widgets.h>
#include <utilities/builders.h>
#include <ShaderGraph/ShaderProperty.h>

namespace ed = ax::NodeEditor;
namespace util = ax::NodeEditor::Utilities;
using namespace REON::SG;

namespace REON::EDITOR {

	struct ShaderGraphLink : public Object {
		ed::PinId startPin;
		ed::PinId endPin;
		ImVec4 color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

		bool operator==(const ShaderGraphLink& other) const {
			return startPin == other.startPin && endPin == other.endPin;
		}
	};

	class ShaderGraph
	{
	public:
		static ShaderGraph& GetInstance() {
			static ShaderGraph instance;
			return instance;
		}

		ShaderGraph() = default;
		~ShaderGraph() = default;

		void initialize();
		void shutdown();
		void render();

		uint64_t GetOrCreateId(const std::string& guid) {
			auto it = guidToIdMap.find(guid);
			if (it != guidToIdMap.end()) return it->second;

			// Generate a new ID (you can use a hash, or just an incrementing counter if preferred)
			uint64_t newId = std::hash<std::string>{}(guid);

			// In case of a rare hash collision, rehash (or add a salt)
			if(idToGuidMap.count(newId) && idToGuidMap[newId] != guid) {
				REON_CRITICAL("Hash collision detected for GUID: {0}. This should be rare, consider using a more robust ID generation strategy.", guid);
			}

			guidToIdMap[guid] = newId;
			idToGuidMap[newId] = guid;
			return newId;
		}

		ed::NodeId MakeNodeId(const std::string& uuid) {
			return ed::NodeId(GetOrCreateId(uuid));
		}
		ed::PinId MakePinId(const std::string& nodeGUID, const std::string& pinName, bool isInput) {
			std::string uuid = nodeGUID + "_" + pinName + (isInput ? "_input" : "_output");
			return ed::PinId(GetOrCreateId(uuid));
		}
		ed::LinkId MakeLinkId(const std::string& uuid) {
			return ed::LinkId(GetOrCreateId(uuid));
		}

		std::string GetGuidFromId(uint64_t id) {
			auto it = idToGuidMap.find(id);
			if (it != idToGuidMap.end()) return it->second;
			return {}; // Not found
		}

		ed::EditorContext* m_Context = nullptr;

		std::vector<ShaderGraphLink> links;
		std::list<SG::ShaderNode> nodes;
		std::unordered_map<std::string, uint64_t> guidToIdMap;
		std::unordered_map<uint64_t, std::string> idToGuidMap;
		std::vector<std::shared_ptr<ShaderProperty>> properties;
		int selectedProperty = -1;
		ed::NodeId selectedNode = 1;

		static std::unordered_map<std::string, const SG::ShaderPin*> pins;

	private:
		ImColor GetIconColor(ShaderValueType type);
		void DrawPinIcon(const ShaderPin& pin, bool connected, int alpha);
		bool CanCreateLink(const SG::ShaderPin* a, const SG::ShaderPin* b);
		const ShaderPin* findPin(ed::PinId id);
		bool findLink(ed::PinId id, ShaderGraphLink&);
		bool findLink(ed::LinkId id, ShaderGraphLink&);
		bool IsPinLinked(ed::PinId id);
		void DrawShaderNode(SG::ShaderNode& node, util::BlueprintNodeBuilder builder);
		void CreateNode(const std::string& templateName);
		void CreateNode(std::shared_ptr<ShaderProperty> property);
		void ChangeNodeVariant(SG::ShaderNode* node, const SG::ShaderNodeVariant& variant);
		bool CanChangeType(const SG::ShaderPin& pin, const ShaderValueType& type, const ShaderNodeVariant*& variant);

		static bool createNewNode;
		static const SG::ShaderPin* newNodeLinkPin;
		static const SG::ShaderPin* newLinkPin;
	};
}