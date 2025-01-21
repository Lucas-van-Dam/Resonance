#pragma once
#include "reonpch.h"
#include "REON/Core.h"

namespace REON {

	enum class EventType {
		None = 0,
		WindowClose, WindowResize, WindowFocus, WindowLostFocus, WindowMoved,
		AppTick, AppUpdate, AppRender,
		KeyPressed, KeyReleased, KeyTyped,
		MouseButtonPressed, MouseButtonReleased, MouseMoved, MouseScrolled,

#ifdef REON_EDITOR
		ButtonClicked, MenuItemSelected, TextboxEdited, /*DropdownSelectionChanged, SliderValueChanged, CheckboxToggled,*/
		ViewportResized, /*ViewportFocusGained, ViewportFocusLost, DockingLayoutChanged, EditorWindowClosed,*/
		/*AssetImported, AssetDeleted, AssetRenamed, AssetModified, AssetOpened,*/
		GameObjectAdded, GameObjectDeleted, GameObjectRenamed, GameObjectSelected, GameObjectDeselected, /*TransformChanged,*/
		ProjectOpened, ProjectSaved, /*FileOpened, FileSaved,*/
		GizmoManipulated, ToolActivated, ToolDeactivated,
		EditorInitialized, EditorShutdown
#endif
	};

	enum EventCategory {
		None = 0,
		EventCategoryApplication = BIT(0),
		EventCategoryInput = BIT(1),
		EventCategoryKeyboard = BIT(2),
		EventCategoryMouse = BIT(3),
		EventCategoryMouseButton = BIT(4),

#ifdef REON_EDITOR
		EventCategoryEditor = BIT(5),
		EventCategoryEditorUI = BIT(6),
		EventCategoryEditorViewportAndWindow = BIT(7),
		EventCategoryEditorAssetManagement = BIT(8),
		EventCategoryEditorObjectManagement = BIT(9),
		EventCategoryEditorProjectManagement = BIT(10),
		EventCategoryEditorTools = BIT(11),
		EventCategoryEditorLifecycle = BIT(12)
#endif
	};

#define EVENT_CLASS_TYPE(type) static REON::EventType GetStaticType() { return REON::EventType::type; }\
								virtual REON::EventType GetEventType() const override { return GetStaticType(); }\
								virtual const char* GetName() const override {	return #type; }

#define EVENT_CLASS_CATEGORY(category) virtual int GetCategoryFlags() const override {return category; }

	class Event {
		friend class EventDispatcher;
	public:
		virtual EventType GetEventType() const = 0;
		virtual const char* GetName() const = 0;
		virtual int GetCategoryFlags() const = 0;
		virtual std::string ToString() const { return GetName(); }

		inline bool IsInCategory(EventCategory category) {
			return GetCategoryFlags() & category;
		}
	public:
		bool Handled = false;
	};

	class EventDispatcher {
		template<typename T>
		using EventFn = std::function<bool(T&)>;
	public:
		EventDispatcher(Event& event)
			: m_Event(event) {}

		template<typename T>
		bool Dispatch(EventFn<T> func)
		{
			if (m_Event.GetEventType() == T::GetStaticType())
			{
				m_Event.Handled = func(*(T*)&m_Event);
				return true;
			}
			return false;
		}

	private:
		Event& m_Event;
	};

	inline std::string format_as(const Event& e) {
		return e.ToString();
	}

}