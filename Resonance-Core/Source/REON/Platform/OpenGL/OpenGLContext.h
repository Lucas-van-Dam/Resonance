#pragma once

#include "REON/Rendering/RenderContext.h"

struct GLFWwindow;

namespace REON {

	class OpenGLContext : public RenderContext
	{
	public:
		OpenGLContext(GLFWwindow* windowHandle);

		void init() override;
		void endFrame() override;

	private:
		GLFWwindow* m_WindowHandle;
	};

}