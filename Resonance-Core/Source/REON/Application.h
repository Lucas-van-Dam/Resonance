#pragma once
#include "REON/Core.h"
#include "REON/Window.h"

#include "REON/Events/ApplicationEvent.h"
#include "REON/LayerStack.h"

#include "REON/ImGui/ImGuiLayer.h"
#include "Rendering/RenderLayer.h"
#include "GameHierarchy/GameLogicLayer.h"
#include "Rendering/RenderContext.h"
#include "ResourceManagement/ResourceManager.h"
#include "EngineServices.h"

namespace REON {

	class Event;

	class  Application
	{
	public:
		Application(const WindowProperties& properties = WindowProperties());
		virtual ~Application();

		void Run();

		void Init(std::filesystem::path assetManifestPath);

		void PushLayer(Layer* layer);
		void PushOverLay(Layer* overlay);

		inline Window& GetWindow() { return *m_Window; }
		inline static Application& Get() { return *s_Instance; }

		uint64_t GetFrameNumber() { return m_FrameNumber; }

		const RenderContext* GetRenderContext() const { return m_Context; }

		inline EngineServices& GetEngineServices()
        {
            return m_EngineServices;
        }

	private:
		void OnWindowClose(const WindowCloseEvent& event);
		void OnWindowResize(const WindowResizeEvent& event);

	private:
		bool m_Running = true;

		RenderContext* m_Context;
		std::unique_ptr<Window> m_Window;
		ImGuiLayer* m_ImGuiLayer;
		RenderLayer* m_RenderLayer;
		GameLogicLayer* m_GameLogicLayer;
		LayerStack m_LayerStack;

		EngineServices m_EngineServices;

		static Application* s_Instance;

		uint64_t m_FrameNumber;

		void OnEvent(const Event& event);
	};

	Application* CreateApplication();

}

