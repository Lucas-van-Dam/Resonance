#pragma once

namespace REON {

	struct ProfileResult {
		std::string name;
		uint64_t startTime;
		uint64_t endTime;
		uint32_t threadID;
	};

	class Profiler
	{
	public:
		static Profiler& Get() {
			static Profiler instance;
			return instance;
		}

		void RecordProfile(const ProfileResult& result);

		void Clear() { 
			m_ProfileResults.clear(); 
		}
		
	private:
		std::mutex m_Mutex;
		std::vector<ProfileResult> m_ProfileResults;
	};

}