#pragma once

#include <functional>
#include <string>
#include <ReflectionSystem.h>
#include "REON/Layer.h"
#include "Windows/Inspector.h"
#include "Windows/SceneHierarchy.h"
#include <future>

#include "ImGuiFileDialog.h"

#include "Reon.h"

#include "Events/ProjectEvent.h"
#include "ProjectManagement/ProjectManager.h"
#include "REON/Events/EventBus.h"
#include "Windows/AssetBrowser.h"


namespace REON::EDITOR {

	class EditorLayer : public REON::Layer {
	public:
		EditorLayer();

		~EditorLayer() override {
			//REON_INFO("EditorLayer destructor called for address: {}", static_cast<void*>(this));
			m_SelectedObject.reset();
		}

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnUpdate() override;
		virtual void OnImGuiRender() override;

		void ProcessKeyPress(const REON::KeyPressedEvent& event);

		void ProcessMouseMove();

		void OnProjectLoaded(const ProjectOpenedEvent& event);

	private:
		void RegisterAsset(const std::filesystem::path& assetPath, const std::filesystem::path& projectPath);

		void CheckAssetsRegistered();

	private:
		std::shared_ptr<REON::GameObject> m_SelectedObject;

		enum class GizmoType {
			Translate,
			Rotate,
			Scale
		};

		GizmoType m_Gizmotype;

		bool m_ProjectLoaded = false;
		bool m_FirstFrame = true;

		bool m_SceneHovered = false;
		double m_SavedX = 0, m_SavedY = 0;
		bool m_CursorLocked = false;

		CallbackID m_KeyPressedCallbackID;
		CallbackID m_ProjectOpenedCallbackID;

		AssetBrowser m_AssetBrowser;

		std::vector<std::future<void>> futures;
		std::future<void> futureCheckingFuture;
	};

}

