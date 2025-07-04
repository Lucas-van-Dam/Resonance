#include "reonpch.h"
#include "Logger.h"

namespace REON {

	std::shared_ptr<spdlog::logger> Logger::s_CoreLogger;
	std::shared_ptr<spdlog::logger> Logger::s_ClientLogger;

	void Logger::Init(const std::string& path) {
		spdlog::set_pattern("%^[%T] %n: %v%$");

		if (!path.empty()) {
			s_CoreLogger = spdlog::basic_logger_mt("REON", path);
			s_ClientLogger = spdlog::basic_logger_mt("APP", path);
		}
		else {
			s_CoreLogger = spdlog::stdout_color_mt("REON");
			s_ClientLogger = spdlog::stdout_color_mt("APP");
		}

		s_CoreLogger->set_level(spdlog::level::trace);
		s_ClientLogger->set_level(spdlog::level::trace);
	}
}