#include "Inspector.h"
#include <glm/ext/vector_float3.hpp>
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>
#include <REON/Math/Quaternion.h>

namespace REON::EDITOR {

	std::unordered_map<std::string, std::function<void(const FieldInfo&, void*)>> Inspector::handlers;

	// Register handler for a specific type
	template<typename T>
	void Inspector::RegisterHandler(const std::string& typeName, std::function<void(const FieldInfo&, void*)> handler) {
		handlers[typeName] = handler;
	}

	void Inspector::Initialize()
	{
		handlers.clear();
		Inspector::RegisterHandler<int>("int", [](const FieldInfo& field, void* instance) {
			int* ptr = reinterpret_cast<int*>(reinterpret_cast<char*>(instance) + field.offset);
			ImGui::InputInt(field.name, ptr);
			});

		Inspector::RegisterHandler<bool>("_Bool", [](const FieldInfo& field, void* instance) {
			bool* ptr = reinterpret_cast<bool*>(reinterpret_cast<char*>(instance) + field.offset);
			ImGui::Checkbox(field.name, ptr);
			});

		Inspector::RegisterHandler<std::string>("std::string", [](const FieldInfo& field, void* instance) {
			std::string* ptr = reinterpret_cast<std::string*>(reinterpret_cast<char*>(instance) + field.offset);

			char buffer[256];
			std::strncpy(buffer, ptr->c_str(), sizeof(buffer));
			buffer[sizeof(buffer) - 1] = '\0';

			if (ImGui::InputText(field.name, buffer, sizeof(buffer))) {
				*ptr = buffer;
			}
			});

		Inspector::RegisterHandler<glm::vec3>("glm::vec3", [](const FieldInfo& field, void* instance) {
			glm::vec3* ptr = reinterpret_cast<glm::vec3*>(reinterpret_cast<char*>(instance) + field.offset);

			// Use ImGui::DragFloat3 for editing glm::vec3
			//static glm::vec3 testVec(1.0f, 2.0f, 3.0f);
			ImGui::DragFloat3(field.name, glm::value_ptr(*ptr), 0.01f);
			});

		Inspector::RegisterHandler<REON::Quaternion>("Quaternion", [](const FieldInfo& field, void* instance) {
			REON::Quaternion* ptr = reinterpret_cast<REON::Quaternion*>(reinterpret_cast<char*>(instance) + field.offset);

			glm::vec3 angles = ptr->getEulerAngles();

			// Use ImGui::DragFloat3 for editing glm::vec3
			//static glm::vec3 testVec(1.0f, 2.0f, 3.0f);
			if (ImGui::DragFloat3(field.name, glm::value_ptr(angles), 0.01f)) {
				ptr->setFromEulerAngles(angles.x, angles.y, angles.z);
			}
			});
	}

	// Render the field using the registered handler
	void Inspector::RenderField(const FieldInfo& field, void* instance) {
		auto it = handlers.find(field.type);
		if (it != handlers.end()) {
			it->second(field, instance);
		}
	}

	// Render all fields of an object
	void Inspector::InspectObject(REON::GameObject& object) {
		ImGui::Begin("Inspector");

		ImGui::Text("Transform");

		auto reflectionClass = *ReflectionRegistry::Instance().GetClass("Transform");

		for (size_t i = 0; i < reflectionClass.field_count; ++i) {
			const FieldInfo& field = reflectionClass.fields[i];
			RenderField(field, object.GetTransform().get());  // Automatically calls the correct handler based on the type
		}

		for (std::shared_ptr<REON::Component> component : object.GetComponents())
		{
			auto name = component->GetTypeName();
			auto reflection = *ReflectionRegistry::Instance().GetClass(name);
			ImGui::Text(name.c_str());
			for (size_t i = 0; i < reflection.field_count; ++i) {
				const FieldInfo& field = reflection.fields[i];
				RenderField(field, component.get());  // Automatically calls the correct handler based on the type
			}
		}

		ImGui::End();
	}

}