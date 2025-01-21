#pragma once

#include "Reon.h"

class ButtonClickedEvent : public REON::Event {
public:
	ButtonClickedEvent(const std::string& id)
		: m_Id(id) {}

	inline const std::string& GetButtonLabel() const { return m_Id; }

	std::string ToString() const override {
		std::stringstream ss;
		ss << "ButtonClickedEvent: " << m_Id;
		return ss.str();
	}

	EVENT_CLASS_TYPE(ButtonClicked)
	EVENT_CLASS_CATEGORY(REON::EventCategoryEditor | REON::EventCategoryEditorUI)

private:
	std::string m_Id;
};

class MenuItemSelectedEvent : public REON::Event {
public:
	MenuItemSelectedEvent(const std::string& id, const std::string& menuItem)
		: m_Id(id), m_MenuItem(menuItem) {}

	inline const std::string& GetMenuItem() const { return m_MenuItem; }

	std::string ToString() const override {
		std::stringstream ss;
		ss << "MenuItemSelectedEvent: " << m_MenuItem;
		return ss.str();
	}

	EVENT_CLASS_TYPE(MenuItemSelected)
	EVENT_CLASS_CATEGORY(REON::EventCategoryEditor | REON::EventCategoryEditorUI)

private:
	std::string m_Id;
	std::string m_MenuItem;
};

class TextboxEditedEvent : public REON::Event {
public:
	TextboxEditedEvent(const std::string& id, const std::string& newText)
		: m_Id(id), m_NewText(newText) {}

	inline const std::string& GetNewText() const { return m_NewText; }

	std::string ToString() const override {
		std::stringstream ss;
		ss << "TextboxEditedEvent: " << m_Id << "changed text to: " << m_NewText;
		return ss.str();
	}

	EVENT_CLASS_TYPE(TextboxEdited)
	EVENT_CLASS_CATEGORY(REON::EventCategoryEditor | REON::EventCategoryEditorUI)

private:
	std::string m_Id;
	std::string m_NewText;
};
