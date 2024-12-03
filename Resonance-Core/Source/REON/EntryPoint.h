#pragma once

#include "../../Reflection/ReflectionRegistry_Registration.h"

#ifdef REON_PLATFORM_WINDOWS

extern REON::Application* REON::CreateApplication();

int main(int argc, char** argv) 
{
	REON::Logger::Init();
	REON_CORE_INFO("Initialized Log");

	auto app = REON::CreateApplication();
	app->Run();
	delete app;
}
#endif