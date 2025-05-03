#include "reonpch.h"
#include "PostProcessingStack.h"
#include "REON/Rendering/RenderManager.h"

namespace REON {
	void PostProcessingStack::Init(int width, int height)
	{
		for (int i = 0; i < 2; ++i) {
			glGenFramebuffers(1, &m_PingPongFbo[i]);
			glBindFramebuffer(GL_FRAMEBUFFER, m_PingPongFbo[i]);

			glGenTextures(1, &m_PingPongTex[i]);
			glBindTexture(GL_TEXTURE_2D, m_PingPongTex[i]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_PingPongTex[i], 0);

			REON_CORE_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Ping-pong FBO is incomplete");
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		for (auto& effect : m_Effects) {
			effect->Init(width, height);
		}
	}
	void PostProcessingStack::AddEffect(std::shared_ptr<PostProcessingEffect> effect)
	{
		m_Effects.push_back(effect);
		GPUTimerQuery timer;
		glGenQueries(1, &timer.startQuery);
		glGenQueries(1, &timer.endQuery);
		m_EffectTimers[effect->GetName()] = timer;
	}

	uint PostProcessingStack::Render(uint inputTexture, uint depthTexture)
	{
		int readIndex = 0;
		int writeIndex = 1;

		uint currentInput = inputTexture;

		for (auto& effect : m_Effects) {
			if (!effect->IsEnabled())
				continue;

			const std::string& name = effect->GetName();
			GPUTimerQuery& timer = m_EffectTimers[name];

			uint outputTexture = m_PingPongTex[writeIndex];
			uint outputFbo = m_PingPongFbo[writeIndex];

			glBindFramebuffer(GL_FRAMEBUFFER, outputFbo);
			glClear(GL_COLOR_BUFFER_BIT);
			glBindVertexArray(RenderManager::QuadVAO);

			glQueryCounter(timer.startQuery, GL_TIMESTAMP);

			effect->Apply(currentInput, depthTexture, outputFbo);

			glQueryCounter(timer.endQuery, GL_TIMESTAMP);

			glBindVertexArray(0);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			// Swap input/output
			currentInput = outputTexture;
			std::swap(readIndex, writeIndex);
		}

		if (m_Profiling) {
			for (auto& [name, query] : m_EffectTimers) {
				GLuint64 startTime, endTime;
				glGetQueryObjectui64v(query.startQuery, GL_QUERY_RESULT, &startTime);
				glGetQueryObjectui64v(query.endQuery, GL_QUERY_RESULT, &endTime);

				double elapsedMs = (endTime - startTime) / 1e6;

				m_EffectTimings[name].push_back(elapsedMs);
			}
		}

		return currentInput;
	}

	void PostProcessingStack::Resize(int width, int height) {
		for (int i = 0; i < 2; ++i) {
			glBindTexture(GL_TEXTURE_2D, m_PingPongTex[i]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, nullptr);
		}

		for (auto& effect : m_Effects) {
			effect->Resize(width, height);
		}
	}
	void PostProcessingStack::HotReloadShaders()
	{
		for (auto& effect : m_Effects) {
			effect->HotReloadShaders();
		}
	}

	void PostProcessingStack::ExportFrameDataToCSV(std::string path, std::string effectName)
	{
		m_Profiling = false;

		std::ofstream out(path);
		if (!out.is_open()) {
			std::cerr << "Failed to open file for writing: " << path << std::endl;
			return;
		}

		out << effectName;

		auto samples = m_EffectTimings[effectName];

		for (double sample : samples)
		{
			out << sample << ",";
		}
	}

	void PostProcessingStack::StartProfiling()
	{
		m_Profiling = true;
	}
}