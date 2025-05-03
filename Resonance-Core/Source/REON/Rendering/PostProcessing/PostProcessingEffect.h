#pragma once
#include "../Shader.h"

namespace REON {
	class PostProcessingEffect
	{
    public:
        virtual ~PostProcessingEffect() = default;

        virtual void Resize(int width, int height) = 0;

        virtual void Apply(uint inputTexture, uint depthTexture, uint outputFbo) = 0;

        virtual void Init(int width, int height) = 0;

        virtual bool IsEnabled() const { return m_Enabled; }
        virtual void SetEnabled(bool enabled) { m_Enabled = enabled; }
        
        virtual void HotReloadShaders() = 0;

        virtual std::string GetName() const = 0;

    protected:
        bool m_Enabled = true;
	};

}