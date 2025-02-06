#include "reonpch.h"
#include "ProfilerTimer.h"

#include "Profiler.h"

namespace REON {
	void ProfilerTimer::Stop() {
		auto endTime = std::chrono::high_resolution_clock::now();
		auto start = std::chrono::duration_cast<std::chrono::microseconds>(m_StartTime.time_since_epoch()).count();
		auto end = std::chrono::duration_cast<std::chrono::microseconds>(endTime.time_since_epoch()).count();

		// Record the profiling result
		Profiler::Get().RecordProfile({ m_Name, static_cast<uint64_t>(start), static_cast<uint64_t>(end), Application::Get().GetFrameNumber() });
		m_Stopped = true;
	}
}