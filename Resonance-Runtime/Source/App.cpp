#include "Reon.h"

#include "REON/EntryPoint.h"
#include "RuntimeLayer.h"
#include "imgui/imgui.h"
//#define _CRTDBG_MAP_ALLOC

class Runtime : public REON::Application
{
public:
	Runtime(const REON::WindowProperties& properties) : REON::Application(properties) {
		PushLayer(new REON::RUNTIME::RuntimeLayer());
	}

	~Runtime(){

	}
};

REON::Application* REON::CreateApplication() {
	WindowProperties properties;

	//Read properties from a config file or similar

	return new Runtime(properties);
}