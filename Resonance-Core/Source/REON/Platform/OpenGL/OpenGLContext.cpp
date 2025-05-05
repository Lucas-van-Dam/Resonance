#include "reonpch.h"
#include "OpenGLContext.h"

#include "GLFW/glfw3.h"
#include <glad/glad.h>

namespace REON {

	OpenGLContext::OpenGLContext(GLFWwindow* windowHandle)
		: m_WindowHandle(windowHandle)
	{
		REON_CORE_ASSERT(windowHandle, "Window handle is null")
	}

	void OpenGLContext::init()
	{
		glfwMakeContextCurrent(m_WindowHandle);
		int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
		REON_CORE_ASSERT(status, "Failed to initialize GLAD");
	}

	void OpenGLContext::swapBuffers()
	{
		glfwSwapBuffers(m_WindowHandle);
	}

}