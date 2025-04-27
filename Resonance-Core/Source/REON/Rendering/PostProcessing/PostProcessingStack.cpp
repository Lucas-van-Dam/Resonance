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
	}

	uint PostProcessingStack::Render(uint inputTexture, uint depthTexture)
	{
		int readIndex = 0;
		int writeIndex = 1;

		uint currentInput = inputTexture;

		for (auto& effect : m_Effects) {
			if (!effect->IsEnabled())
				continue;

			uint outputTexture = m_PingPongTex[writeIndex];
			uint outputFbo = m_PingPongFbo[writeIndex];

			glBindFramebuffer(GL_FRAMEBUFFER, outputFbo);
			glClear(GL_COLOR_BUFFER_BIT);
			glBindVertexArray(RenderManager::QuadVAO);

			effect->Apply(currentInput, depthTexture, outputFbo);

			glBindVertexArray(0);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			// Swap input/output
			currentInput = outputTexture;
			std::swap(readIndex, writeIndex);
		}

		return currentInput;
	}

	void PostProcessingStack::Resize(int width, int height){
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
}