#include "reonpch.h"
#include "Profiler.h"

namespace REON {

	void Profiler::RecordProfile(const ProfileResult& result)
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		m_ProfileResults.push_back(result);
	}
}