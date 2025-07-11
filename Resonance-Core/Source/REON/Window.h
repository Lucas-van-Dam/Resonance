#pragma once

#include "REON/Core.h"

#include <string>
#include <functional>

namespace REON {

	class Event;

	struct WindowProperties {
		std::string Title;
		unsigned int Width;
		unsigned int Height;
		bool VSync;
		bool Resizable;
		bool FullScreen;

		WindowProperties(const std::string& title = "Resonance",
			unsigned int width = 1200,
			unsigned int height = 800,
			bool vSync = false,
			bool resizable = true,
			bool fullScreen = false)
			: Title(title), Width(width), Height(height), VSync(vSync), Resizable(resizable), FullScreen(fullScreen) {}
	};

	class Window {
	public:
		using EventCallBackFn = std::function<void(const Event&)>;

		virtual ~Window() {};

		virtual void PollEvents() = 0;

		virtual unsigned int GetWidth() const = 0;
		virtual unsigned int GetHeight() const = 0;

		virtual void SetEventCallback(const EventCallBackFn& callback) = 0;
		virtual void SetVSync(bool enabled) = 0;
		virtual bool IsVSync() const = 0;
		virtual void SetFullScreen(bool enabled) = 0;
		virtual bool IsFullScreen() const = 0;
		virtual void SetResizable(bool enabled) = 0;
		virtual bool IsResizable() const = 0;

		virtual void* GetNativeWindow() const = 0;

		static Window* Create(const WindowProperties& properties = WindowProperties());
	};

}