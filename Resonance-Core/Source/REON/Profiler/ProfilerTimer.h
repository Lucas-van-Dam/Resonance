#pragma once
#include <chrono>
#include "Profiler.h"

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

		void Stop() {
			auto endTime = std::chrono::high_resolution_clock::now();
			auto start = std::chrono::duration_cast<std::chrono::microseconds>(m_StartTime.time_since_epoch()).count();
			auto end = std::chrono::duration_cast<std::chrono::microseconds>(endTime.time_since_epoch()).count();

			// Record the profiling result
			Profiler::Get().RecordProfile({ m_Name, static_cast<uint64_t>(start), static_cast<uint64_t>(end), GetThreadID() });
			m_Stopped = true;
		}

	private:
		uint32_t GetThreadID() {
			return static_cast<uint32_t>(GetCurrentThreadId());
		}

		const char* m_Name;
		std::chrono::time_point<std::chrono::high_resolution_clock> m_StartTime;
		bool m_Stopped;
	};

}