#pragma once

#include "Reon.h"

class ViewportResizedEvent : public REON::Event {
public:
	ViewportResizedEvent(int width, int height)
		: m_Width(width), m_Height(height) {}

	inline int GetWidth() const { return m_Width; }
	inline int GetHeight() const { return m_Height; }

	std::string ToString() const override {
		std::stringstream ss;
		ss << "ViewportResizedEvent: " << m_Width << "x" << m_Height;
		return ss.str();
	}

	EVENT_CLASS_TYPE(ViewportResized)
	EVENT_CLASS_CATEGORY(REON::EventCategoryEditor | REON::EventCategoryEditorViewportAndWindow)

private:
	int m_Width, m_Height;
};