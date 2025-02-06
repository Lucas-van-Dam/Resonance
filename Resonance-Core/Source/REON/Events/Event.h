#pragma once
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

#define EVENT_CLASS_NAME(type) virtual const char* GetName() const override {	return #type; }

	class Event {
	public:
		virtual const char* GetName() const = 0;
		virtual std::string ToString() const { return GetName(); }
		virtual ~Event() = default;
		virtual const std::type_info& getDynamicType() const {
			return typeid(*this);
		}
	};

	inline std::string format_as(const Event& e) {
		return e.ToString();
	}

}