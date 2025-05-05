#include "reonpch.h"
#include "WindowsWindow.h"

#include "REON/Events/ApplicationEvent.h"
#include "REON/Events/KeyEvent.h"
#include "REON/Events/MouseEvent.h"

#include "REON/Platform/Vulkan/VulkanContext.h"

namespace REON {

	static bool s_GLFWInitialized = false;

	Window* Window::Create(const WindowProperties& properties) {
		return new WindowsWindow(properties);
	}

	WindowsWindow::WindowsWindow(const WindowProperties& properties)
	{
		Init(properties);
	}

	WindowsWindow::~WindowsWindow() {
		ShutDown();
	}

	void WindowsWindow::Init(const WindowProperties& properties) {
		m_Data.Title = properties.Title;
		m_Data.Width = properties.Width;
		m_Data.Height = properties.Height;

		REON_CORE_INFO("Creating window  {0} ({1}, {2})", m_Data.Title, m_Data.Width, m_Data.Height);

		if (!s_GLFWInitialized) {
			int success = glfwInit();

			REON_CORE_ASSERT(success, "Could not initialize GLFW")

			s_GLFWInitialized = true;
		}

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		m_Window = glfwCreateWindow(m_Data.Width, m_Data.Height, m_Data.Title.c_str(), nullptr, nullptr);

		m_Context = new VulkanContext;
		m_Context->init();

		glfwSetWindowUserPointer(m_Window, &m_Data);
		SetVSync(false);

		glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height) {

			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			data.Width = width;
			data.Height = height;

			WindowResizeEvent event(width, height);
			data.EventCallBack(event);
			});

		glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window) {
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			WindowCloseEvent event;
			data.EventCallBack(event);
			});

		glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			switch (action) {
			case GLFW_PRESS:
			{
				KeyPressedEvent event(key, 0);
				data.EventCallBack(event);
				break;
			}
			case GLFW_REPEAT:
			{
				KeyPressedEvent event(key, 1);
				data.EventCallBack(event);
				break;
			}
			case GLFW_RELEASE:
			{
				KeyReleasedEvent event(key);
				data.EventCallBack(event);
				break;
			}
			}
			});

		glfwSetCharCallback(m_Window, [](GLFWwindow* window, unsigned int key) {
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			KeyTypedEvent event(key);
			data.EventCallBack(event);
			});

		glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int mods) {
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			switch (action) {
			case GLFW_PRESS: {
				MouseButtonPressedEvent event(button);
				data.EventCallBack(event);
				break;
			}
			case GLFW_RELEASE: {
				MouseButtonReleasedEvent event(button);
				data.EventCallBack(event);
				break;
			}
			}
			});

		glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xOffset, double yOffset) {
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			MouseScrolledEvent event((float)xOffset, (float)yOffset);
			data.EventCallBack(event);
			});

		glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double xPos, double yPos) {
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			MouseMovedEvent event((float)xPos, (float)yPos);
			data.EventCallBack(event);
			});
	}

	void WindowsWindow::ShutDown() {
		glfwDestroyWindow(m_Window);
		glfwTerminate();
		s_GLFWInitialized = false;
		m_Context->cleanup();
	}



	void WindowsWindow::OnUpdate() {
		glfwPollEvents();
		m_Context->swapBuffers();
	}

	void WindowsWindow::SetVSync(bool enabled) {
		if (enabled) {
			glfwSwapInterval(1);
		}
		else {
			glfwSwapInterval(0);
		}

		m_Data.VSync = enabled;
	}

	bool WindowsWindow::IsVSync() const {
		return m_Data.VSync;
	}

}
