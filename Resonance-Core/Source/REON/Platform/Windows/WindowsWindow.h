#pragma once

#include "REON/Window.h"

#include "REON/Logger.h"

#include "GLFW/glfw3.h"
#include <REON/Rendering/RenderContext.h>

namespace REON {

	struct WindowData {
		std::string Title;
		unsigned int Width = 0, Height = 0;
		bool VSync = false;

		std::function<void(Event&)> EventCallBack;
	};

	class  WindowsWindow : public Window {
	public:
		WindowsWindow(const WindowProperties& properties);
		virtual ~WindowsWindow();

		void OnUpdate() override;
		void OnResize() override;

		inline unsigned int GetWidth() const override { return m_Data.Width; }
		inline unsigned int GetHeight() const override { return m_Data.Height; }

		inline void SetEventCallback(const EventCallBackFn& callback) override { m_Data.EventCallBack = callback; }
		void SetVSync(bool enabled) override;
		bool IsVSync() const override;

		inline void* GetNativeWindow() const override { return m_Window; }
	private:
		virtual void Init(const WindowProperties& properties);
		virtual void ShutDown();
	private:
		GLFWwindow* m_Window;

		RenderContext* m_Context;

		WindowData m_Data;
	};

}

