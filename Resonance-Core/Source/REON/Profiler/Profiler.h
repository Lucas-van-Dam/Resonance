#pragma once

#include "REON/Events/EventBus.h"
#include "REON/Events/ApplicationEvent.h"

namespace REON {

	class ProfilerBuffer;

	class ProfileBufferRegistry {
	public:
		static std::mutex RingBufferRegistryMutex;
		static std::vector<ProfilerBuffer*>& GetRingBufferRegistry() {
			static std::vector<ProfilerBuffer*> registry;
			return registry;
		}
	};

	struct ProfileResult {
		std::string name;
		uint64_t startTime;
		uint64_t endTime;
		uint64_t frameNumber;
	};

	class Profiler
	{
	public:
		static Profiler& Get() {
			static Profiler instance;
			return instance;
		}

		Profiler() {
			//EventBus::Get().subscribe<FrameEndEvent>([this](const FrameEndEvent& event) {Clear(); });
		}

		void RecordProfile(const ProfileResult& result);

		void Clear();
		
	private:
		std::mutex m_Mutex;
		
	};

}