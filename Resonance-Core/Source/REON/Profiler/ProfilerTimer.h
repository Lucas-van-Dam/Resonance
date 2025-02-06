#pragma once
#include <chrono>
#include "REON/Application.h"

namespace REON {

	class ProfilerTimer
	{
	public:
		ProfilerTimer(const char* name) 
		: m_Name(name), m_Stopped(false) {
			m_StartTime = std::chrono::high_resolution_clock::now();
		}

		~ProfilerTimer() {
			if (!m_Stopped)
				Stop();
		}

		void Stop();

	private:
		const char* m_Name;
		std::chrono::time_point<std::chrono::high_resolution_clock> m_StartTime;
		bool m_Stopped;
	};

}