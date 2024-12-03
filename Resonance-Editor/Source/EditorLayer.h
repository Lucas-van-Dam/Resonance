#pragma once

#include <functional>
#include <string>
#include <ReflectionSystem.h>
#include "REON/Layer.h"

#include "Reon.h"
//#include "REON/Core.h"

class EditorLayer : public REON::Layer {
public:
	EditorLayer();

	~EditorLayer() override {
		//REON_INFO("EditorLayer destructor called for address: {}", static_cast<void*>(this));
		m_SelectedObject.reset();
		pendingReparentSource.reset();
		pendingReparentTarget.reset();
		handlers.clear();
	}

	virtual void OnAttach() {}
	virtual void OnDetach() {}
	virtual void OnUpdate() override;
	virtual void OnImGuiRender() override;
	virtual void OnEvent(REON::Event & event) override;

	// Template to register handlers based on type
	template<typename T>
	static void RegisterHandler(const std::string& typeName, std::function<void(const FieldInfo&, void*)> handler);

	// Function to render a field based on the registered handler
	static void RenderField(const FieldInfo& field, void* instance);

	void RenderGameObjectNode(std::shared_ptr<REON::GameObject> gameObject, std::shared_ptr<REON::GameObject>& selectedObject);

	void ProcessReparenting();

	void RenderSceneHierarchy(const std::vector<std::shared_ptr<REON::GameObject>>& rootObjects, std::shared_ptr<REON::GameObject>& selectedObject);

	// Render fields dynamically based on reflection data
	static void InspectObject(REON::GameObject& object);

	bool ProcessKeyPress(REON::KeyPressedEvent& event);

	void ProcessMouseMove();

private:
	// Static registry for handlers, maps type names to handler functions
	static std::unordered_map<std::string, std::function<void(const FieldInfo&, void*)>> handlers;

	std::shared_ptr<REON::GameObject> m_SelectedObject;

	std::shared_ptr<REON::GameObject> pendingReparentSource = nullptr;
	std::shared_ptr<REON::GameObject> pendingReparentTarget = nullptr;

	bool m_SceneHovered = false;
	double m_SavedX = 0, m_SavedY = 0;
	bool m_CursorLocked = false;
};


