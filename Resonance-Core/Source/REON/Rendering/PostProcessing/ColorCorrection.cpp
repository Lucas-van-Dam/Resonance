#include "reonpch.h"
#include "ColorCorrection.h"
#include "REON/Rendering/RenderManager.h"

namespace REON {
	void ColorCorrection::Resize(int width, int height)
	{
	}

	void ColorCorrection::Apply(uint inputTexture, uint depthTexture, uint outputFbo)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, outputFbo);
		glDisable(GL_DEPTH_TEST);
		glClear(GL_COLOR_BUFFER_BIT);
		m_ScreenShader->use();
		glBindVertexArray(RenderManager::QuadVAO);
		glActiveTexture(GL_TEXTURE0);
		glUniform1i(glGetUniformLocation(m_ScreenShader->ID, "screenTexture"), 0);
		glBindTexture(GL_TEXTURE_2D, inputTexture);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glEnable(GL_DEPTH_TEST);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void ColorCorrection::Init(int width, int height)
	{
	}

	void ColorCorrection::HotReloadShaders()
	{
		m_ScreenShader->ReloadShader();
	}

}
