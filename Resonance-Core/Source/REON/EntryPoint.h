#pragma once

#include "REON/Application.h"

#ifdef REON_PLATFORM_WINDOWS

extern REON::Application* REON::CreateApplication();

int main(int argc, char** argv) 
{
    REON::Logger::Init("");
	REON_CORE_INFO("Initialized Log");

	REON_CORE_INFO("Working Directory: {0}", std::filesystem::current_path().string());

	auto app = REON::CreateApplication();
	app->Run();
	delete app;
}
#endif