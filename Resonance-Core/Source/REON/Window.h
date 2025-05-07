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

		WindowProperties(const std::string& title = "Resonance",
			unsigned int width = 1200,
			unsigned int height = 800)
			: Title(title), Width(width), Height(height) {}
	};

	class Window {
	public:
		using EventCallBackFn = std::function<void(const Event&)>;

		virtual ~Window() {};

		virtual void OnUpdate() = 0;
		virtual void OnResize() = 0;

		virtual unsigned int GetWidth() const = 0;
		virtual unsigned int GetHeight() const = 0;

		virtual void SetEventCallback(const EventCallBackFn& callback) = 0;
		virtual void SetVSync(bool enabled) = 0;
		virtual bool IsVSync() const = 0;

		virtual void* GetNativeWindow() const = 0;

		static Window* Create(const WindowProperties& properties = WindowProperties());
	};

}