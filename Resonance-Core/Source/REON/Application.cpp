#include "reonpch.h"
#include "Application.h"

#include "Input.h"

#include "glad/glad.h"
#include "REON/GameHierarchy/SceneManager.h"


namespace REON {

	Application* Application::s_Instance = nullptr;

	Application::Application()
	{
		REON_CORE_ASSERT(!s_Instance, "Application already exists")
		s_Instance = this;

		m_Window = std::unique_ptr<Window>(Window::Create());
		m_Window->SetEventCallback(REON_BIND_EVENT_FN(Application::OnEvent));

		m_GameLogicLayer = new GameLogicLayer();
		PushLayer(m_GameLogicLayer);

		m_RenderLayer = new RenderLayer();
		PushLayer(m_RenderLayer);
		
		m_ImGuiLayer = new ImGuiLayer();
		PushOverLay(m_ImGuiLayer);
	}

	Application::~Application() {

	}

	void Application::OnEvent(Event& event) {
		EventDispatcher dispatcher(event);
		dispatcher.Dispatch<WindowCloseEvent>(REON_BIND_EVENT_FN(Application::OnWindowClose));

		//REON_CORE_TRACE("{0}", event);

		for (auto it = m_LayerStack.end(); it != m_LayerStack.begin(); ) {
			(*--it)->OnEvent(event);
			if (event.Handled)
				break;
		}
	}

	void Application::PushLayer(Layer* layer)
	{
		m_LayerStack.PushLayer(layer);
		layer->OnAttach();
	}

	void Application::PushOverLay(Layer* overlay)
	{
		m_LayerStack.PushOverlay(overlay);
		overlay->OnAttach();
	}

	void Application::Run() {


		while (m_Running) {
			glClear(GL_COLOR_BUFFER_BIT);
			for (Layer* layer : m_LayerStack)
				layer->OnUpdate();

			m_ImGuiLayer->Begin();
			for (Layer* layer : m_LayerStack)
				layer->OnImGuiRender();
			m_ImGuiLayer->End();

			m_Window->OnUpdate();

			SceneManager::Get()->GetCurrentScene()->ProcessGameObjectDeletion();
		}
	}

	bool Application::OnWindowClose(WindowCloseEvent& event)
	{
		m_Running = false;
		return true;
	}

}
