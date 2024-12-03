#include "EditorLayer.h"
#include "imgui/imgui.h"
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "ImGuizmo.h"
#include "REON/GameHierarchy/Components/Transform.h"


std::unordered_map<std::string, std::function<void(const FieldInfo&, void*)>> EditorLayer::handlers;

// Register handler for a specific type
template<typename T>
void EditorLayer::RegisterHandler(const std::string& typeName, std::function<void(const FieldInfo&, void*)> handler) {
	handlers[typeName] = handler;
}

EditorLayer::EditorLayer() : Layer("Inspector")
{

	EditorLayer::RegisterHandler<int>("int", [](const FieldInfo& field, void* instance) {
		int* ptr = reinterpret_cast<int*>(reinterpret_cast<char*>(instance) + field.offset);
		ImGui::InputInt(field.name, ptr);
		});

	EditorLayer::RegisterHandler<bool>("_Bool", [](const FieldInfo& field, void* instance) {
		bool* ptr = reinterpret_cast<bool*>(reinterpret_cast<char*>(instance) + field.offset);
		ImGui::Checkbox(field.name, ptr);
		});

	EditorLayer::RegisterHandler<std::string>("std::string", [](const FieldInfo& field, void* instance) {
		std::string* ptr = reinterpret_cast<std::string*>(reinterpret_cast<char*>(instance) + field.offset);

		char buffer[256];
		std::strncpy(buffer, ptr->c_str(), sizeof(buffer));
		buffer[sizeof(buffer) - 1] = '\0';

		if (ImGui::InputText(field.name, buffer, sizeof(buffer))) {
			*ptr = buffer;
		}
		});

	EditorLayer::RegisterHandler<glm::vec3>("glm::vec3", [](const FieldInfo& field, void* instance) {
		glm::vec3* ptr = reinterpret_cast<glm::vec3*>(reinterpret_cast<char*>(instance) + field.offset);

		// Use ImGui::DragFloat3 for editing glm::vec3
		//static glm::vec3 testVec(1.0f, 2.0f, 3.0f);
		ImGui::DragFloat3(field.name, glm::value_ptr(*ptr), 0.01f);
		});

	EditorLayer::RegisterHandler<REON::Quaternion>("Quaternion", [](const FieldInfo& field, void* instance) {
		REON::Quaternion* ptr = reinterpret_cast<REON::Quaternion*>(reinterpret_cast<char*>(instance) + field.offset);

		glm::vec3 angles = ptr->getEulerAngles();

		// Use ImGui::DragFloat3 for editing glm::vec3
		//static glm::vec3 testVec(1.0f, 2.0f, 3.0f);
		if (ImGui::DragFloat3(field.name, glm::value_ptr(angles), 0.01f)) {
			ptr->setFromEulerAngles(angles.x, angles.y, angles.z);
		}
		});
}

void EditorLayer::OnUpdate()
{
	ProcessMouseMove();
	if(!m_SelectedObject)
		m_SelectedObject = REON::SceneManager::Get()->GetCurrentScene()->GetGameObject(0);
}

void EditorLayer::OnImGuiRender()
{
	//ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	//ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));

	if (ImGui::Begin("Scene", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse))
	{
		auto scene = REON::SceneManager::Get()->GetCurrentScene();

		auto size = ImGui::GetContentRegionAvail();
		scene->renderManager->SetRenderDimensions(size.x, size.y);
		m_SceneHovered = ImGui::IsWindowHovered();

		ImGui::Image((void*)(intptr_t)scene->renderManager->GetEndBuffer(), size, ImVec2(0, 1), ImVec2(1, 0));

		ImVec2 windowSize = ImGui::GetWindowSize();
		ImVec2 pos = ImGui::GetWindowPos();

		ImGuizmo::SetDrawlist();

		ImGuizmo::SetRect(pos.x, pos.y, windowSize.x, windowSize.y);

		glm::mat4 viewMatrix = scene->GetEditorCamera()->GetViewMatrix();
		glm::mat4 projMatrix = scene->GetEditorCamera()->GetProjectionMatrix();
		glm::mat4 objectMatrix = scene->GetGameObject(0)->GetTransform()->GetTransformationMatrix();

		if (ImGuizmo::Manipulate(glm::value_ptr(viewMatrix), glm::value_ptr(projMatrix),
			ImGuizmo::TRANSLATE, ImGuizmo::WORLD, glm::value_ptr(objectMatrix)))
		{
			scene->GetGameObject(0)->GetTransform()->SetWorldTransform(objectMatrix);
		}
	}
	ImGui::End();

	//ImGui::PopStyleVar(2);

	InspectObject(*m_SelectedObject);
	RenderSceneHierarchy(REON::SceneManager::Get()->GetCurrentScene()->GetRootObjects(), m_SelectedObject);
}

void EditorLayer::OnEvent(REON::Event& event)
{
	REON::EventDispatcher dispatcher(event);
	dispatcher.Dispatch<REON::KeyPressedEvent>(REON_BIND_EVENT_FN(EditorLayer::ProcessKeyPress));
}

// Render the field using the registered handler
void EditorLayer::RenderField(const FieldInfo& field, void* instance) {
	auto it = handlers.find(field.type);
	if (it != handlers.end()) {
		it->second(field, instance);
	}
}

// Recursive function to render the hierarchy
void EditorLayer::RenderGameObjectNode(std::shared_ptr<REON::GameObject> gameObject, std::shared_ptr<REON::GameObject>& selectedObject) {
	// Check if the node should be open in the tree view
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;

	// Highlight the selected object
	if (gameObject == selectedObject) {
		flags |= ImGuiTreeNodeFlags_Selected;
	}

	// Render the tree node for this game object
	bool nodeOpen = ImGui::TreeNodeEx(gameObject->GetName().c_str(), flags);

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

void EditorLayer::ProcessReparenting() {
	if (pendingReparentSource && pendingReparentTarget) {
		// Add the source as a child of the new parent
		pendingReparentTarget->AddChild(pendingReparentSource);

		// Clear the pending operations
		pendingReparentSource.reset();
		pendingReparentTarget.reset();
	}
}

// Function to render the entire scene hierarchy
void EditorLayer::RenderSceneHierarchy(const std::vector<std::shared_ptr<REON::GameObject>>& rootObjects, std::shared_ptr<REON::GameObject>& selectedObject) {
	ImGui::Begin("Scene Hierarchy");

	for (const auto& rootObject : rootObjects) {
		RenderGameObjectNode(rootObject, selectedObject);
	}

	ProcessReparenting();

	ImGui::End();
}

// Render all fields of an object
void EditorLayer::InspectObject(REON::GameObject& object) {
	ImGui::Begin("Inspector");

	ImGui::Text("Transform");

	auto reflectionClass = *ReflectionRegistry::Instance().GetClass("Transform");

	for (size_t i = 0; i < reflectionClass.field_count; ++i) {
		const FieldInfo& field = reflectionClass.fields[i];
		RenderField(field, object.GetTransform().get());  // Automatically calls the correct handler based on the type
	}

	

	for(std::shared_ptr<REON::Component> component : object.GetComponents())
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


bool EditorLayer::ProcessKeyPress(REON::KeyPressedEvent& event)
{
	if (event.GetKeyCode() == REON_KEY_F2 && event.GetRepeatCount() == 0) {
		REON::SceneManager::Get()->GetCurrentScene()->renderManager->HotReloadShaders();
	}
	return false;
}

void EditorLayer::ProcessMouseMove()
{
	if (m_SceneHovered) {
		if (REON::Input::IsMouseButtonPressed(REON_MOUSE_BUTTON_2)) {
			if (!m_CursorLocked) {
				glfwGetCursorPos((GLFWwindow*)REON::Application::Get().GetWindow().GetNativeWindow(), &m_SavedX, &m_SavedY);
				glfwSetInputMode((GLFWwindow*)REON::Application::Get().GetWindow().GetNativeWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			}

			float Xoffset = REON::Input::GetMouseX() - (float)m_SavedX;
			float Yoffset = m_SavedY - REON::Input::GetMouseY();
			REON::SceneManager::Get()->GetCurrentScene()->GetEditorCamera()->ProcessMouseMovement(Xoffset, Yoffset);
			glfwSetCursorPos((GLFWwindow*)REON::Application::Get().GetWindow().GetNativeWindow(), m_SavedX, m_SavedY);
			m_CursorLocked = true;
		}
		else {
			glfwSetInputMode((GLFWwindow*)REON::Application::Get().GetWindow().GetNativeWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			m_CursorLocked = false;
		}
	}
}
