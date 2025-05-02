#include "reonpch.h"
#include "BloomEffect.h"
#include <REON/Rendering/RenderManager.h>

namespace REON {
    float BloomEffect::FilterRadius = 0.005f;
    float BloomEffect::bloomStrength = 0.04f;
    float BloomEffect::bloomThreshold = 1.1f;

    void BloomEffect::Init(int width, int height)
    {
        if (m_Init) return;
        m_SrcViewportSize = glm::ivec2(width, height);
        m_SrcViewportSizeFloat = glm::vec2((float)width, (float)height);

        // Framebuffer
        const unsigned int num_bloom_mips = 5;
        bool status = m_FBO.Init(width, height, num_bloom_mips);
        REON_CORE_ASSERT(status, "Bloom FBO is not complete!");

        // Downsample
        m_DownsampleShader->use();
        glm::vec2 texelSize = glm::vec2(1.0f / width, 1.0f / height);
        for (int i = 0; i < num_bloom_mips; ++i)
        {
            m_DownsampleShader->setVec2("srcTexelSize[" + std::to_string(i) + "]", texelSize);
            texelSize *= 2.0f;
        }
        m_DownsampleShader->setInt("srcTexture", 0);

        // Upsample
        m_UpsampleShader->use();
        float aspectRatio = width / height;
        m_UpsampleShader->setFloat("aspectRatio", aspectRatio);
        m_UpsampleShader->setInt("srcTexture", 0);

        m_Init = true;
    }

    void BloomEffect::RenderDownsamples(unsigned int srcTexture)
    {
        const std::vector<BloomMip>& mipChain = m_FBO.MipChain();

        m_DownsampleShader->use();

        // Bind srcTexture (HDR color buffer) as initial texture input
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, srcTexture);

        glDisable(GL_BLEND);

        // Progressively downsample through the mip chain
        for (int i = 0; i < mipChain.size(); i++)
        {
            const BloomMip& mip = mipChain[i];
            glViewport(0, 0, mip.size.x, mip.size.y);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                GL_TEXTURE_2D, mip.texture, 0);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            m_DownsampleShader->setInt("uMipLevel", i);

            // Render screen-filled quad of resolution of current mip
            RenderManager::RenderFullScreenQuad();
            
            // Set current mip as texture input for next iteration
            glBindTexture(GL_TEXTURE_2D, mip.texture);
        }
    }

    void BloomEffect::RenderUpsamples(float filterRadius)
    {
        const std::vector<BloomMip>& mipChain = m_FBO.MipChain();

        m_UpsampleShader->use();
        m_UpsampleShader->setFloat("filterRadius", FilterRadius);

        // Enable additive blending
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        glBlendEquation(GL_FUNC_ADD);

        for (int i = mipChain.size() - 1; i > 0; i--)
        {
            const BloomMip& mip = mipChain[i];
            const BloomMip& nextMip = mipChain[i - 1];

            glm::vec2 texelSize = glm::vec2(1.0f / mip.size.x, 1.0f / mip.size.y);

            m_UpsampleShader->setVec2("texelSize", texelSize);
            m_UpsampleShader->setFloat("mipWeight", GetWeightForMip(i));

            // Bind viewport and texture from where to read
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mip.texture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            if (i == mipChain.size() - 1) {
                m_UpsampleShader->setBool("firstMip", true);
            }
            else {
                m_UpsampleShader->setBool("firstMip", false);
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, mipChain[i + 1].texture);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            }

            // Set framebuffer render target (we write to this texture)
            glViewport(0, 0, nextMip.size.x, nextMip.size.y);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                GL_TEXTURE_2D, nextMip.texture, 0);

            // Render screen-filled quad of resolution of current mip
            RenderManager::RenderFullScreenQuad();
        }

        // Disable additive blending
        glDisable(GL_BLEND);
        //glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // Restore if this was default
    }

    float BloomEffect::GetWeightForMip(int mipLevel) {
        const float weights[] = { 0.4f, 0.3f, 0.2f, 0.1f, 0.05f };
        return weights[std::min(mipLevel, 4)];
    }

    void BloomEffect::Resize(int width, int height)
    {
        m_SrcViewportSize = { width, height };
        m_SrcViewportSizeFloat = { static_cast<float>(width), static_cast<float>(height) };
        m_FBO.Resize(width, height);
        m_DownsampleShader->use();
        glm::vec2 texelSize = glm::vec2(1.0f / width, 1.0f / height);
        for (int i = 0; i < 5; ++i)
        {
            m_DownsampleShader->setVec2("srcTexelSize[" + std::to_string(i) + "]", texelSize);
            texelSize *= 2.0f;
        }

        m_UpsampleShader->use();
        float aspectRatio = m_SrcViewportSizeFloat.x / m_SrcViewportSizeFloat.y;
        m_UpsampleShader->setFloat("aspectRatio", aspectRatio);
    }

    void BloomEffect::Apply(uint inputTexture, uint depthTexture, uint outputFbo)
    {
        RenderBloomTexture(inputTexture, FilterRadius);

        glBindFramebuffer(GL_FRAMEBUFFER, outputFbo);

        m_CompositeShader->use();
        m_CompositeShader->setFloat("bloomStrength", bloomStrength);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, inputTexture);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_FBO.MipChain()[0].texture);

        RenderManager::RenderFullScreenQuad();
    }

    void BloomEffect::HotReloadShaders()
    {
        m_CompositeShader->ReloadShader();
        m_UpsampleShader->ReloadShader();
        m_DownsampleShader->ReloadShader();
    }

    BloomEffect::BloomEffect() : m_Init(false) {}
    BloomEffect::~BloomEffect() {}

    void BloomEffect::Destroy()
    {
        m_FBO.Destroy();
        m_Init = false;
    }

    void BloomEffect::RenderBloomTexture(unsigned int srcTexture, float filterRadius)
    {
        m_FBO.BindForWriting();

        this->RenderDownsamples(srcTexture);
        this->RenderUpsamples(filterRadius);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        // Restore viewport
        glViewport(0, 0, m_SrcViewportSize.x, m_SrcViewportSize.y);
    }

    GLuint BloomEffect::BloomTexture()
    {
        return m_FBO.MipChain()[0].texture;
    }
}