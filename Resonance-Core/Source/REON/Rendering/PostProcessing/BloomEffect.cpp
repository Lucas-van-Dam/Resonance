#include "reonpch.h"
#include "BloomEffect.h"
#include <REON/Rendering/RenderManager.h>

namespace REON {
	void BloomEffect::Resize(int width, int height)
	{
		glBindTexture(GL_TEXTURE_2D, m_BloomTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, NULL);
		glBindTexture(GL_TEXTURE_2D, m_BlurTextureHorizontal);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, NULL);
		glBindTexture(GL_TEXTURE_2D, m_BlurTextureVertical);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, NULL);
	}

	void BloomEffect::Apply(uint inputTexture, uint depthTexture, uint outputFbo)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_BloomFbo);
		glDisable(GL_DEPTH_TEST);
		glClear(GL_COLOR_BUFFER_BIT);

		// Use the bright-pass shader
		m_BrightPassShader->use();
		glActiveTexture(GL_TEXTURE0);
		glUniform1i(glGetUniformLocation(m_BrightPassShader->ID, "uScene"), 0);
		glUniform1f(glGetUniformLocation(m_BrightPassShader->ID, "threshold"), BloomThreshold);
		glBindTexture(GL_TEXTURE_2D, inputTexture);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		for (int i = 0; i < BloomPasses; ++i) {
			// Step 2: Apply horizontal blur
			glBindFramebuffer(GL_FRAMEBUFFER, m_BlurFboHorizontal);
			glClear(GL_COLOR_BUFFER_BIT);
			m_BlurPassShader->use();
			glUniform1i(glGetUniformLocation(m_BlurPassShader->ID, "brightPassTexture"), 0);
			m_BlurPassShader->setVec2("offset", glm::vec2(1.0f, 0.0f));
			glBindTexture(GL_TEXTURE_2D, m_BloomTexture);
			glDrawArrays(GL_TRIANGLES, 0, 6);

			// Step 3: Apply vertical blur
			glBindFramebuffer(GL_FRAMEBUFFER, m_BlurFboVertical);
			glClear(GL_COLOR_BUFFER_BIT);
			m_BlurPassShader->use();
			glUniform1i(glGetUniformLocation(m_BlurPassShader->ID, "brightPassTexture"), 0);
			m_BlurPassShader->setVec2("offset", glm::vec2(0.0f, 1.0f));
			glBindTexture(GL_TEXTURE_2D, m_BlurTextureHorizontal);
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}

		// Step 4: Composite the bloom with the original scene
		glBindFramebuffer(GL_FRAMEBUFFER, outputFbo);
		glClear(GL_COLOR_BUFFER_BIT);
		m_CompositeShader->use();
		glActiveTexture(GL_TEXTURE0);
		glUniform1i(glGetUniformLocation(m_CompositeShader->ID, "sceneTexture"), 0);
		glBindTexture(GL_TEXTURE_2D, inputTexture);
		glActiveTexture(GL_TEXTURE1);
		glUniform1i(glGetUniformLocation(m_CompositeShader->ID, "bloomTexture"), 1);
		m_CompositeShader->setFloat("bloomStrength", BloomStrength);
		glBindTexture(GL_TEXTURE_2D, m_BlurTextureVertical);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glEnable(GL_DEPTH_TEST);
	}

	void BloomEffect::Init(int width, int height)
	{	
		RenderManager::InitializeFboAndTexture(m_BloomFbo, m_BloomTexture, width, height);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		RenderManager::InitializeFboAndTexture(m_BlurFboHorizontal, m_BlurTextureHorizontal, width, height);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		RenderManager::InitializeFboAndTexture(m_BlurFboVertical, m_BlurTextureVertical, width, height);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
}