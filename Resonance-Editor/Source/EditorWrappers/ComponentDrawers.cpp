#include "ComponentDrawers.h"
#include "imgui.h"
#include "DrawFuncs.h"
#include <REON/ResourceManagement/ResourceManager.h>
#include <ProjectManagement/ProjectManager.h>
#include <Commands/CommandManager.h>
#include <Commands/PropertyChangeCommand.h>

namespace REON::EDITOR {

	void ComponentDrawers::DrawInspector_Transform(std::shared_ptr<Transform> transform)
	{
		static bool wasChangingPosition = false;
		if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
			DrawFuncs::DrawVec3WithUndoRedo("position", transform->localPosition, 
				[transform](const glm::vec3& pos) { transform->localPosition = pos; },
				[transform]() { return transform->localPosition; }, 0.01f, 100.0f);

			if (transform->eulerDirty) {
				transform->eulerCache = transform->localRotation.getEulerAngles();
				transform->eulerDirty = false;
			}

			glm::vec3 eulerDegrees = glm::degrees(transform->eulerCache);
			if (DrawFuncs::DrawVec3WithUndoRedo("Rotation", eulerDegrees,
				[transform](const glm::vec3& rot) {transform->eulerCache = glm::radians(rot);
			transform->localRotation.setFromEulerAngles(transform->eulerCache);
			transform->eulerDirty = false; },
				[transform]() {return glm::degrees(transform->eulerCache); })) {
				//transform->eulerCache = glm::radians(eulerDegrees);
				//transform->localRotation.setFromEulerAngles(transform->eulerCache);
				//transform->eulerDirty = false;
			}

			DrawFuncs::DrawVec3WithUndoRedo("Scale", transform->localScale,
				[transform](const glm::vec3& scale) {transform->localScale = scale; },
				[transform]() {return transform->localScale; }, 0.01);
		}
		ImGui::Dummy(ImVec2(0.0f, 4.0f));
	}

	void ComponentDrawers::DrawInspector_Light(std::shared_ptr<Light> light)
	{
		if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen)) {
			DrawFuncs::DrawColorEdit("Color", light->color);
			ImGui::DragFloat("Intensity", &light->intensity, 0.01f, 0.0f, 100.0f);
		}
		ImGui::Dummy(ImVec2(0.0f, 4.0f));
	}
	void ComponentDrawers::DrawInspector_Renderer(std::shared_ptr<Renderer> renderer)
	{
		if (ImGui::CollapsingHeader("Renderer", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Columns(2, "RendererColumns");
			ImGui::SetColumnWidth(0, 100.0f);
			ImGui::Text("%s", "Mesh");
			ImGui::NextColumn();

		//	auto mesh = renderer->mesh.Get<Mesh>();

		//	const char* meshName = nullptr;

		//	if (mesh)
		//		meshName = mesh->GetName().empty() ? ("###" + mesh->GetID()).c_str() : mesh->GetName().c_str();
		//	else
		//		meshName = (std::string("No Mesh##") + renderer->GetID()).c_str();

		//	ImGui::Button(meshName, ImVec2(-1, 0));
		//	ImGui::Columns(1);
		//	ImGui::Columns(2, "Materials");
		//	ImGui::SetColumnWidth(0, 100.0f);
		//	ImGui::Text("materials");
		//	ImGui::NextColumn();

		//	for (int i = 0; i < renderer->materials.size(); i++) {
		//		auto mat = renderer->materials[i].Get<Material>();

		//		ImGui::Button((mat->GetName() + "##" + mat->GetID() + std::to_string(i)).c_str(), ImVec2(-1, 0));

		//		if (ImGui::BeginDragDropTarget()) {
		//			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_BROWSER_ITEM")) {
		//				std::string filePath((char*)payload->Data);
		//				if (AssetInfo* info = AssetRegistry::Instance().GetAssetByPath(filePath)) {
		//					if (info->type == "material") {

		//						if (auto material = ResourceManager::GetInstance().GetResource<Material>(info->id)) {
		//							renderer->SetMaterial(i, material);
		//						}
		//						else {
		//							auto materialPtr = std::dynamic_pointer_cast<Material>(ResourceManager::GetInstance().GetResourceFromAsset(info, ProjectManager::GetInstance().GetCurrentProjectPath()));
		//							ResourceManager::GetInstance().AddResource(materialPtr);
		//							renderer->SetMaterial(i, materialPtr);
		//						}
		//					}
		//				}
		//			}
		//			ImGui::EndDragDropTarget();
		//		}
		//	}
		//	ImGui::Columns(1);
		}
		ImGui::Dummy(ImVec2(0.0f, 4.0f));
	}
}