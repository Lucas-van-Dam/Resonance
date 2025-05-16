#pragma once

#include <string>
#include <sstream>
#include "Event.h"

namespace REON {

	class  WindowResizeEvent : public Event {
	public:
		WindowResizeEvent(unsigned int width, unsigned int height)
			: m_Width(width), m_Height(height) {}

		inline unsigned int GetWidth() const { return m_Width; }
		inline unsigned int GetHeight() const { return m_Height; }

		std::string ToString() const override
		{
			std::stringstream ss;
			ss << "WindowResizeEvent: " << m_Width << ", " << m_Height;
			return ss.str();
		}

		EVENT_CLASS_NAME(WindowResize)

	private:
		unsigned int m_Width, m_Height;
	};

	class ContextResizeEvent : public Event {
	public:
		ContextResizeEvent(unsigned int width, unsigned int height)
			: m_Width(width), m_Height(height) {}

		inline unsigned int getWidth() const { return m_Width; }
		inline unsigned int getHeight() const { return m_Height; }

		std::string ToString() const override {
			std::stringstream ss;
			ss << "ContextResizeEvent: " << m_Width << ", " << m_Height;
			return ss.str();
		}

		EVENT_CLASS_NAME(ContextResize)

	private:
		unsigned int m_Width, m_Height;
	};

	class  WindowCloseEvent : public Event {
	public:
		WindowCloseEvent() {}

		EVENT_CLASS_NAME(WindowClose)
	};

	class  AppTickEvent : public Event {
	public:
		AppTickEvent() {}

		EVENT_CLASS_NAME(AppTick)
	};

	class AppUpdateEvent : public Event {
	public:
		AppUpdateEvent() {}

		EVENT_CLASS_NAME(AppUpdate)
	};

	class AppRenderEvent : public Event {
	public:
		AppRenderEvent() {}

		EVENT_CLASS_NAME(AppRender)
	};


	class FrameStartEvent : public Event {
	public:
		FrameStartEvent() {}

		EVENT_CLASS_NAME(FrameStart)
	};

	class FrameEndEvent : public Event {
	public:
		FrameEndEvent() {}

		EVENT_CLASS_NAME(FrameEnd)
	};

}