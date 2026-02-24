#include "SceneHierarchy.h"
#include "ProjectManagement/ProjectManager.h"
#include "REON/ResourceManagement/loaders/ModelLoader.h"
#include "AssetManagement/AssetRegistry.h"

namespace REON::EDITOR {

	std::shared_ptr<REON::GameObject> SceneHierarchy::pendingReparentSource;
	std::shared_ptr<REON::GameObject> SceneHierarchy::pendingReparentTarget;

	// Recursive function to render the hierarchy
	void SceneHierarchy::RenderGameObjectNode(std::shared_ptr<REON::GameObject> gameObject, std::shared_ptr<REON::GameObject>& selectedObject) {
		// Check if the node should be open in the tree view
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;

		// Highlight the selected object
		if (gameObject == selectedObject) {
			flags |= ImGuiTreeNodeFlags_Selected;
		}

		bool nodeOpen = false;

		// Render the tree node for this game object
		std::string display = gameObject->GetName() + "##" + gameObject->GetID();
		if (gameObject->GetChildren().size() > 0)
			nodeOpen = ImGui::TreeNodeEx(display.c_str(), flags);
		else
			ImGui::Button(display.c_str());

		// Handle selection logic
		if (ImGui::IsItemClicked()) {
			selectedObject = gameObject; // Update the selected object
		}

		// Drag source: Start dragging the current GameObject
		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
			// Payload contains a pointer to the GameObject being dragged
			ImGui::SetDragDropPayload("GAMEOBJECT", &gameObject, sizeof(std::shared_ptr<REON::GameObject>));
			ImGui::Text("Dragging: %s", gameObject->GetName().c_str());
			pendingReparentSource = gameObject; // Track the source being dragged
			ImGui::EndDragDropSource();
		}

		// Drag target: Accept a GameObject being dropped
		if (ImGui::BeginDragDropTarget()) {
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("GAMEOBJECT")) {
				auto droppedGameObject = *static_cast<std::shared_ptr<REON::GameObject>*>(payload->Data);

				// Validate the operation (prevent self or circular parenting)
				if (droppedGameObject != gameObject && !gameObject->IsDescendantOf(droppedGameObject)) {
					pendingReparentTarget = gameObject; // Track the target for reparenting
				}
			}
			ImGui::EndDragDropTarget();
		}

		// If the node is open, recursively render its children
		if (nodeOpen) {
			for (auto& child : gameObject->GetChildren()) {
				RenderGameObjectNode(child, selectedObject);
			}
			ImGui::TreePop(); // Close the tree node
		}
	}

	void SceneHierarchy::ProcessReparenting() {
		if (pendingReparentSource && pendingReparentTarget) {
			// Add the source as a child of the new parent
			pendingReparentTarget->AddChild(pendingReparentSource);

			// Clear the pending operations
			pendingReparentSource.reset();
			pendingReparentTarget.reset();
		}
	}

	// Function to render the entire scene hierarchy
	void SceneHierarchy::RenderSceneHierarchy(const std::vector<std::shared_ptr<REON::GameObject>>& rootObjects, std::shared_ptr<REON::GameObject>& selectedObject) {
		ImGui::Begin("Scene Hierarchy");
		
		for (const auto& rootObject : rootObjects) {
			RenderGameObjectNode(rootObject, selectedObject);
		}

		ImGui::Dummy(ImGui::GetContentRegionAvail());

		if (ImGui::BeginDragDropTarget()) {	
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_BROWSER_ITEM")) {
				std::string filePath((char*)payload->Data);
				auto extension = std::filesystem::path(filePath).extension();
                if (extension == ".glb" || extension == ".gltf")
                {
					REON_INFO("Dropped file: \"{}\" into scene", filePath);
                    const auto& records = AssetRegistry::Instance().FindBySource(filePath);
                    const auto& it = std::find_if(records.begin(), records.end(),
                                 [](const AssetRecord& record) { return record.type == ASSET_MODEL; });
                    ModelLoader::LoadModelFromFile(it->id,
                                                   SceneManager::Get()->GetCurrentScene());
					Model::ConstructGameObjectFromModelFile(ProjectManager::GetInstance().GetCurrentProjectPath() + "\\" + filePath, SceneManager::Get()->GetCurrentScene());
				}
			}
			ImGui::EndDragDropTarget();
		}

		ProcessReparenting();

		ImGui::End();
	}

}
