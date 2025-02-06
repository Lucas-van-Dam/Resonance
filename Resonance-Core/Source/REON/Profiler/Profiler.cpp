#include "reonpch.h"
#include "Profiler.h"

#include "ProfilerBuffer.h"

namespace REON {

	std::mutex ProfileBufferRegistry::RingBufferRegistryMutex;

	void Profiler::RecordProfile(const ProfileResult& result)
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		g_ProfilerBuffer.Push(result);
	}
	void Profiler::Clear()
	{
		g_ProfilerBuffer.Clear();
	}
}