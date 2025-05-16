#pragma once
#include "REON/Core.h"
#include "REON/Window.h"

#include "REON/Events/ApplicationEvent.h"
#include "REON/LayerStack.h"

#include "REON/ImGui/ImGuiLayer.h"
#include "Rendering/RenderLayer.h"
#include "GameHierarchy/GameLogicLayer.h"
#include "Rendering/RenderContext.h"

namespace REON {

	class Event;

	class  Application
	{
	public:
		Application();
		virtual ~Application();

		void Run();

		void PushLayer(Layer* layer);
		void PushOverLay(Layer* overlay);

		inline Window& GetWindow() { return *m_Window; }
		inline static Application& Get() { return *s_Instance; }

		uint64_t GetFrameNumber() { return m_FrameNumber; }

		const RenderContext* GetRenderContext() const { return m_Context; }

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

		static Application* s_Instance;

		uint64_t m_FrameNumber;

		void OnEvent(const Event& event);
	};

	Application* CreateApplication();

}

