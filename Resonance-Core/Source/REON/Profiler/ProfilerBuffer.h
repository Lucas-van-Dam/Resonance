#pragma once

namespace REON {
	struct ProfileResult;

	class ProfilerBuffer
	{
	public:
		ProfilerBuffer(size_t capacity);

		~ProfilerBuffer();

		void Push(const ProfileResult& record);

		void Clear();

		const std::vector<ProfileResult>& getBuffer() const { return m_Buffer; }
		size_t size() const { return m_Count; }
		size_t capacity() const { return m_Capacity; }
		size_t head() const { return m_Head; }

	private:
		size_t m_Capacity;
		std::vector<ProfileResult> m_Buffer;
		size_t m_Head;
		size_t m_Count;
	};

	extern thread_local ProfilerBuffer g_ProfilerBuffer;
}