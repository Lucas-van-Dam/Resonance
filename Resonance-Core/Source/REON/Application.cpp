#include "reonpch.h"
#include "Application.h"

#include "Input.h"

#include "glad/glad.h"
#include "REON/GameHierarchy/SceneManager.h"
#include "REON/Events/EventBus.h"
#include "REON/Events/Event.h"

#include "public/tracy/Tracy.hpp"

#include "REON/Platform/Vulkan/VulkanContext.h"


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
		EventBus::Get().subscribe<WindowResizeEvent>(REON_BIND_EVENT_FN(Application::OnWindowResize));

		m_Window = std::unique_ptr<Window>(Window::Create());
		m_Window->SetEventCallback(REON_BIND_EVENT_FN(Application::OnEvent));

		m_Context = new VulkanContext(static_cast<GLFWwindow*>(m_Window->GetNativeWindow()));
		m_Context->init();

		m_GameLogicLayer = new GameLogicLayer();
		PushLayer(m_GameLogicLayer);

		m_RenderLayer = new RenderLayer();
		PushLayer(m_RenderLayer);
		//
		m_ImGuiLayer = new ImGuiLayer(static_cast<VulkanContext*>(m_Context));
		PushOverLay(m_ImGuiLayer);
	}

	Application::~Application() {
		vkDeviceWaitIdle(static_cast<VulkanContext*>(m_Context)->getDevice());
		for (Layer* layer : m_LayerStack)
			layer->OnCleanup();
		//SceneManager::Get()->Destroy();
		ResourceManager::GetInstance().Destroy();
		m_Context->cleanup();
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
				m_Context->startFrame();
				////glClear(GL_COLOR_BUFFER_BIT);
				for (Layer* layer : m_LayerStack)
					layer->OnUpdate();

				//m_Context->render();
				m_ImGuiLayer->Begin();
				for (Layer* layer : m_LayerStack)
					layer->OnImGuiRender();
				m_ImGuiLayer->End();

				m_Window->PollEvents();
				m_Context->endFrame();

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

	void Application::OnWindowResize(const WindowResizeEvent& event)
	{
		m_Context->resize();
	}

	void Application::OnEvent(const Event& event)
	{
		EventBus::Get().publishDynamic(event);
	}

}
