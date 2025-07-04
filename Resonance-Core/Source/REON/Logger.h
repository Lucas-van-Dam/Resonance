#pragma once

#include "Core.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/fmt/ostr.h"

namespace REON {

	class  Logger
	{
	public:
		static void Init(const std::string& logPath = "");

		inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }

	private:
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_ClientLogger;
	};

}

#ifdef REON_DEBUG
#define REON_CORE_CRITICAL(...) ::REON::Logger::GetCoreLogger()->critical(__VA_ARGS__)
#define REON_CORE_ERROR(...) ::REON::Logger::GetCoreLogger()->error(__VA_ARGS__)
#define REON_CORE_WARN(...) ::REON::Logger::GetCoreLogger()->warn(__VA_ARGS__)
#define REON_CORE_INFO(...) ::REON::Logger::GetCoreLogger()->info(__VA_ARGS__)
#define REON_CORE_TRACE(...) ::REON::Logger::GetCoreLogger()->trace(__VA_ARGS__)
#else
#define REON_CORE_CRITICAL(...)
#define REON_CORE_ERROR(...) 
#define REON_CORE_WARN(...)
#define REON_CORE_INFO(...)
#define REON_CORE_TRACE(...)
#endif

#define REON_CRITICAL(...) ::REON::Logger::GetClientLogger()->critical(__VA_ARGS__)
#define REON_ERROR(...) ::REON::Logger::GetClientLogger()->error(__VA_ARGS__)
#define REON_WARN(...) ::REON::Logger::GetClientLogger()->warn(__VA_ARGS__)
#define REON_INFO(...) ::REON::Logger::GetClientLogger()->info(__VA_ARGS__)
#define REON_TRACE(...) ::REON::Logger::GetClientLogger()->trace(__VA_ARGS__)