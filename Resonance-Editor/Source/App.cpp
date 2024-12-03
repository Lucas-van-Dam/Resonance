#include "Reon.h"
#include "EditorLayer.h"

#include "REON/EntryPoint.h"

#include "imgui/imgui.h"
#include "EditorLayer.h"

//#define _CRTDBG_MAP_ALLOC

class Editor : public REON::Application
{
public:
	Editor() {
		PushLayer(new EditorLayer());
	}

	~Editor(){

	}
};

REON::Application* REON::CreateApplication() {
	//_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	return new Editor();
}