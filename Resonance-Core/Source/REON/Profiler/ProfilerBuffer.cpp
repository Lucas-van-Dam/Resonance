#include "reonpch.h"
#include "ProfilerBuffer.h"

#include "Profiler.h"

namespace REON {
	ProfilerBuffer::ProfilerBuffer(size_t capacity)
		: m_Capacity(capacity), m_Buffer(capacity), m_Head(0), m_Count(0)
	{
		std::lock_guard<std::mutex> lock(ProfileBufferRegistry::RingBufferRegistryMutex);
		ProfileBufferRegistry::GetRingBufferRegistry().push_back(this);
	}

	ProfilerBuffer::~ProfilerBuffer() {
		std::lock_guard<std::mutex> lock(ProfileBufferRegistry::RingBufferRegistryMutex);
		auto& registry = ProfileBufferRegistry::GetRingBufferRegistry();
		registry.erase(std::remove(registry.begin(), registry.end(), this), registry.end());
	}

	void ProfilerBuffer::Push(const ProfileResult& record) {
		m_Buffer[m_Head] = record;
		m_Head = (m_Head + 1) % m_Capacity;
		if (m_Count < m_Capacity) {
			++m_Count;
		}
	}

	void ProfilerBuffer::Clear() {
		m_Buffer.clear();
		m_Head = 0;
		m_Count = 0;
	}

	thread_local ProfilerBuffer g_ProfilerBuffer(1000);
}