#include "reonpch.h"
#include "DepthOfField.h"
#include "../RenderManager.h"

namespace REON {
    float DepthOfField::m_FocusDistance = 10;
    float DepthOfField::m_FocusRange = 1.0;

	void DepthOfField::Resize(int width, int height)
	{
        m_BlurShader->use();
        m_BlurShader->setVec2("texelSize", glm::vec2(1.0f / width, 1.0f / height));

        glBindTexture(GL_TEXTURE_2D, m_CoCTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, width, height, 0, GL_RED, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

        glBindFramebuffer(GL_FRAMEBUFFER, m_CoCFbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_CoCTexture, 0);

        glBindTexture(GL_TEXTURE_2D, m_BlurTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

        glBindFramebuffer(GL_FRAMEBUFFER, m_BlurFbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_BlurTexture, 0);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void DepthOfField::Apply(uint inputTexture, uint depthTexture, uint outputFbo)
	{
        glBindFramebuffer(GL_FRAMEBUFFER, m_CoCFbo);

        m_CoCShader->use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, depthTexture);

        m_CoCShader->setFloat("focusDistance", m_FocusDistance);
        m_CoCShader->setFloat("focusRange", m_FocusRange);

        RenderManager::RenderFullScreenQuad();


        glBindFramebuffer(GL_FRAMEBUFFER, m_BlurFbo);

        m_BlurShader->use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, inputTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_CoCTexture);

        RenderManager::RenderFullScreenQuad();


        glBindFramebuffer(GL_FRAMEBUFFER, outputFbo);

        m_CompositeShader->use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, inputTexture);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_BlurTexture);

        glActiveTexture(GL_TEXTURE2);    
        glBindTexture(GL_TEXTURE_2D, m_CoCTexture);

        RenderManager::RenderFullScreenQuad();
	}

	void DepthOfField::Init(int width, int height)
	{
        glGenFramebuffers(1, &m_CoCFbo);
        glBindFramebuffer(GL_FRAMEBUFFER, m_CoCFbo);

        glGenTextures(1, &m_CoCTexture);
        glBindTexture(GL_TEXTURE_2D, m_CoCTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, width, height, 0, GL_RED, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_CoCTexture, 0);

        REON_CORE_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "framebuffer is not complete!");


        glGenFramebuffers(1, &m_BlurFbo);
        glBindFramebuffer(GL_FRAMEBUFFER, m_BlurFbo);

        glGenTextures(1, &m_BlurTexture);
        glBindTexture(GL_TEXTURE_2D, m_BlurTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_BlurTexture, 0);

        REON_CORE_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "framebuffer is not complete!");

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        m_BlurShader->use();
        m_BlurShader->setVec2("texelSize", glm::vec2(1.0f / width, 1.0f / height));
	}

	void DepthOfField::HotReloadShaders()
	{
        m_CoCShader->ReloadShader();
        m_BlurShader->ReloadShader();
        m_CompositeShader->ReloadShader();
	}
    std::string DepthOfField::GetName() const
    {
        return "Depth of Field";
    }
}