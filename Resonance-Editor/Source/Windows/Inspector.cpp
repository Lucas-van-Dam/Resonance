#include "Inspector.h"
#include <glm/ext/vector_float3.hpp>
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>
#include <REON/Math/Quaternion.h>
#include <EditorWrappers/AssetDrawers.h>
#include <EditorWrappers/ComponentDrawers.h>

namespace REON::EDITOR {

	std::unordered_map<std::string, std::function<void(const FieldInfo&, void*)>> Inspector::handlers;
	std::unordered_map<std::string, std::function<void(const std::shared_ptr<GameObject>&)>> Inspector::componentMap;

	// Register handler for a specific type
	template<typename T>
	void Inspector::RegisterHandler(const std::string& typeName, std::function<void(const FieldInfo&, void*)> handler) {
		handlers[typeName] = handler;
	}

	void Inspector::Initialize()
	{
		componentMap.clear();
		componentMap["Light"] = [](const std::shared_ptr<GameObject>& object) {
			auto light = std::make_shared<Light>();
			object->AddComponent<Light>(light);
			};

		componentMap["Renderer"] = [](const std::shared_ptr<GameObject>& object) {
			auto renderer = std::make_shared<Renderer>();
			object->AddComponent<Renderer>(renderer);
			};

		componentMap["Camera"] = [](const std::shared_ptr<GameObject>& object) {
			auto camera = std::make_shared<Camera>();
			object->AddComponent<Camera>(camera);
			};

		handlers.clear();
		Inspector::RegisterHandler<int>("int", [](const FieldInfo& field, void* instance) {
			int* ptr = reinterpret_cast<int*>(field.getter(instance));
			ImGui::InputInt(field.name, ptr);
			});

		Inspector::RegisterHandler<float>("float", [](const FieldInfo& field, void* instance) {
			float* ptr = reinterpret_cast<float*>(field.getter(instance));
			ImGui::DragFloat(field.name, ptr, 0.1f);
			});

		Inspector::RegisterHandler<bool>("bool", [](const FieldInfo& field, void* instance) {
			bool* ptr = reinterpret_cast<bool*>(field.getter(instance));
			ImGui::Checkbox(field.name, ptr);
			});

		Inspector::RegisterHandler<std::string>("std::string", [](const FieldInfo& field, void* instance) {
			std::string* ptr = reinterpret_cast<std::string*>(field.getter(instance));

			char buffer[256];
			std::strncpy(buffer, ptr->c_str(), sizeof(buffer));
			buffer[sizeof(buffer) - 1] = '\0';

			if (ImGui::InputText(field.name, buffer, sizeof(buffer))) {
				*ptr = buffer;
			}
			});

		Inspector::RegisterHandler<glm::vec3>("glm::vec3", [](const FieldInfo& field, void* instance) {
			glm::vec3* ptr = reinterpret_cast<glm::vec3*>(field.getter(instance));

			// Use ImGui::DragFloat3 for editing glm::vec3
			//static glm::vec3 testVec(1.0f, 2.0f, 3.0f);
			ImGui::DragFloat3(field.name, glm::value_ptr(*ptr), 0.01f);
			});

		Inspector::RegisterHandler<REON::Quaternion>("Quaternion", [](const FieldInfo& field, void* instance) {
			REON::Quaternion* ptr = reinterpret_cast<REON::Quaternion*>(field.getter(instance));

			glm::vec3 angles = ptr->getEulerAngles();

			// Use ImGui::DragFloat3 for editing glm::vec3
			//static glm::vec3 testVec(1.0f, 2.0f, 3.0f);
			if (ImGui::DragFloat3(field.name, glm::value_ptr(angles), 0.01f)) {
				Quaternion quat;
				quat.setFromEulerAngles(angles.x, angles.y, angles.z);
				quat.Normalize();
				*ptr = quat;
			}
			});
	}

	// Render the field using the registered handler
	void Inspector::RenderField(const FieldInfo& field, void* instance) {
		auto it = handlers.find(field.type);
		if (it != handlers.end()) {
			it->second(field, instance);
		}
		else {
			ImGui::Text(field.name);
		}
	}

#define DISPATCH_DRAW_INSPECTOR(pointer, type)				\
		do{													\
			auto wrapper = EditorWrapper_##type(pointer);	\
			wrapper.DrawInspector();						\
		}while(0)											\

	// Render all fields of an object
	void Inspector::InspectObject(std::shared_ptr<GameObject>& object) {
		ImGui::Begin("Inspector");
		if (!object) {
			ImGui::End();
			return;
		}

		ComponentDrawers::DrawInspector_Transform(object->GetTransform());

		for (std::shared_ptr<REON::Component> component : object->GetComponents())
		{
			auto name = component->GetTypeName();
			if (name == "Transform") continue;
			if (name == "Light") {
				ComponentDrawers::DrawInspector_Light(std::dynamic_pointer_cast<REON::Light>(component));
				continue;
			}
			else if (name == "Renderer") {
				ComponentDrawers::DrawInspector_Renderer(std::dynamic_pointer_cast<Renderer>(component));
				continue;
			}
			else {
				continue;
			}

			auto reflection = *ReflectionRegistry::Instance().GetClass(name);
			ImGui::Text(name.c_str());
			for (size_t i = 0; i < reflection.field_count; ++i) {
				const FieldInfo& field = reflection.fields[i];
				RenderField(field, component.get());  // Automatically calls the correct handler based on the type
			}
		}
		const float buttonWidth = 150.0f;
		const float contentWidth = ImGui::GetContentRegionAvail().x;
		const float xOffset = (contentWidth - buttonWidth) * 0.5f;

		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + xOffset);
		if (ImGui::Button("Add Component", ImVec2(buttonWidth, 0))) {
			ImGui::OpenPopup("AddComponentPopup");
		}

		if (ImGui::BeginPopup("AddComponentPopup")) {
			for (const auto& [name, createFunc] : componentMap) {
				if (!object->HasComponent(name) && ImGui::MenuItem(name.c_str())) {
					createFunc(object);
				}
			}
			ImGui::EndPopup();
		}

		ImGui::End();
	}

	void Inspector::InspectObject(std::filesystem::path assetPath)
	{
		ImGui::Begin("Inspector");
		auto extension = assetPath.extension().string();

		if (extension == ".gltf" || extension == ".glb") {
			AssetDrawers::DrawInspector_Model(assetPath);
		}
		else if (extension == ".material") {
			AssetDrawers::DrawInspector_Material(assetPath);
		}

		ImGui::End();
	}

}