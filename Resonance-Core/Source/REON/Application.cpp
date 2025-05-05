#include "reonpch.h"
#include "Application.h"

#include "Input.h"

#include "glad/glad.h"
#include "REON/GameHierarchy/SceneManager.h"
#include "REON/Events/EventBus.h"
#include "REON/Events/Event.h"

#include "public/tracy/Tracy.hpp"

void* operator new(std::size_t count)
{
	auto ptr = malloc(count);
	TracyAlloc(ptr, count);
	return ptr;
}
void operator delete(void* ptr) noexcept
{
	TracyFree(ptr);
	free(ptr);
}

namespace REON {

	Application* Application::s_Instance = nullptr;

	Application::Application()
	{
		REON_CORE_ASSERT(!s_Instance, "Application already exists")
		s_Instance = this;

		EventBus::Get().subscribe<WindowCloseEvent>(REON_BIND_EVENT_FN(Application::OnWindowClose));

		m_Window = std::unique_ptr<Window>(Window::Create());
		m_Window->SetEventCallback(REON_BIND_EVENT_FN(Application::OnEvent));

		//m_GameLogicLayer = new GameLogicLayer();
		//PushLayer(m_GameLogicLayer);

		//m_RenderLayer = new RenderLayer();
		//PushLayer(m_RenderLayer);
		//
		//m_ImGuiLayer = new ImGuiLayer();
		//PushOverLay(m_ImGuiLayer);
	}

	Application::~Application() {
		
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
		m_FrameNumber = 0;
		while (m_Running) {
			m_FrameNumber++;
			FrameStartEvent frameStartEvent;
			EventBus::Get().publish(frameStartEvent);
			{
				////glClear(GL_COLOR_BUFFER_BIT);
				//for (Layer* layer : m_LayerStack)
				//	layer->OnUpdate();

				//m_ImGuiLayer->Begin();
				//for (Layer* layer : m_LayerStack)
				//	layer->OnImGuiRender();
				//m_ImGuiLayer->End();

				m_Window->OnUpdate();

				if(auto scene = SceneManager::Get()->GetCurrentScene())
					scene->ProcessGameObjectAddingAndDeletion();
			}
			FrameEndEvent frameEndEvent;
			EventBus::Get().publish(frameEndEvent);
			FrameMark;
		}
	}

	void Application::OnWindowClose(const WindowCloseEvent& event)
	{
		m_Running = false;
	}

	void Application::OnEvent(const Event& event)
	{
		EventBus::Get().publishDynamic(event);
	}

}
