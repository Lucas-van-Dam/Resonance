#pragma once

#include <functional>
#include <string>
#include <ReflectionSystem.h>
#include "REON/Layer.h"
#include "Inspector.h"
#include "SceneHierarchy.h"

#include "ImGuiFileDialog.h"

#include "Reon.h"
//#include "REON/Core.h"

#include "Events/ProjectEvent.h"
#include "ProjectManagement/ProjectManager.h"

namespace REON::EDITOR {

	class EditorLayer : public REON::Layer {
	public:
		EditorLayer();

		~EditorLayer() override {
			//REON_INFO("EditorLayer destructor called for address: {}", static_cast<void*>(this));
			m_SelectedObject.reset();
		}

		virtual void OnAttach() override {}
		virtual void OnDetach() override {}
		virtual void OnUpdate() override;
		virtual void OnImGuiRender() override;
		virtual void OnEvent(REON::Event& event) override;

		bool ProcessKeyPress(REON::KeyPressedEvent& event);

		void ProcessMouseMove();

		bool OnProjectLoaded(ProjectOpenedEvent& event);

	private:
		std::shared_ptr<REON::GameObject> m_SelectedObject;

		bool projectLoaded = false;

		bool m_SceneHovered = false;
		double m_SavedX = 0, m_SavedY = 0;
		bool m_CursorLocked = false;
	};

}

