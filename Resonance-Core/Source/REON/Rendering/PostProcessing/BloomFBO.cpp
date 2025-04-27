#include "reonpch.h"
#include "BloomFBO.h"
#include "glad/glad.h"

namespace REON {

    BloomFBO::BloomFBO() : m_Init(false) {}
    BloomFBO::~BloomFBO() {}

    void BloomFBO::Destroy()
    {
        for (int i = 0; i < m_MipChain.size(); i++) {
            glDeleteTextures(1, &m_MipChain[i].texture);
            m_MipChain[i].texture = 0;
        }
        glDeleteFramebuffers(1, &m_FBO);
        m_FBO = 0;
        m_Init = false;
    }

    void BloomFBO::BindForWriting()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);
    }

    const std::vector<BloomMip>& BloomFBO::MipChain() const
    {
        return m_MipChain;
    }

    void BloomFBO::Resize(uint windowWidth, uint windowHeight)
    {
        glm::ivec2 mipIntSize = {windowWidth, windowHeight};
        glm::vec2  mipSize = { static_cast<float>(windowWidth), static_cast<float>(windowHeight) };

        for (int i = 0; i < m_MipChain.size(); ++i)
        {
            BloomMip& mip = m_MipChain[i];

            mipSize *= 0.5f;
            mipIntSize /= 2;

            mip.size = mipSize;
            mip.intSize = mipIntSize;

            glBindTexture(GL_TEXTURE_2D, mip.texture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R11F_G11F_B10F, mipIntSize.x, mipIntSize.y, 0, GL_RGB, GL_FLOAT, nullptr);
        }
    }

    bool BloomFBO::Init(uint windowWidth, uint windowHeight, uint mipChainLength)
    {
        if (m_Init) return true;

        glGenFramebuffers(1, &m_FBO);
        glBindFramebuffer(GL_FRAMEBUFFER, m_FBO);

        glm::vec2 mipSize(windowWidth, windowHeight);
        glm::ivec2 mipIntSize(windowWidth, windowHeight);

        for (int i = 0; i < mipChainLength; i++) {
            BloomMip mip;

            mipSize *= 0.5;
            mipIntSize /= 2;

            mip.size = mipSize;
            mip.intSize = mipIntSize;

            glGenTextures(1, &mip.texture);
            glBindTexture(GL_TEXTURE_2D, mip.texture);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_R11F_G11F_B10F, mipIntSize.x, mipIntSize.y, 0, GL_RGB, GL_FLOAT, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            m_MipChain.emplace_back(mip);
        }

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D, m_MipChain[0].texture, 0);

        unsigned int attachments[1] = { GL_COLOR_ATTACHMENT0 };
        glDrawBuffers(1, attachments);

        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
        {
            REON_CORE_ERROR("Framebuffer incomplete: {0}", status);
            return false;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        m_Init = true;
        return true;
    }
}