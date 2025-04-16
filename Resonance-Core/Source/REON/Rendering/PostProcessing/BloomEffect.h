#pragma once
#include "PostProcessingEffect.h"
#include "REON/ResourceManagement/ResourceManager.h"

namespace REON {
	class BloomEffect : public PostProcessingEffect
	{
	public:
		void Resize(int width, int height) override;
		void Apply(uint inputTexture, uint depthTexture, uint outputFbo) override;
		void Init(int width, int height) override;

	public:
		int BloomPasses = 10;
		float BloomThreshold = 0.95f;
		float BloomStrength = 0.5f;

	private:
		uint m_BloomFbo, m_BloomTexture;
		uint m_BlurFboHorizontal, m_BlurTextureHorizontal;
		uint m_BlurFboVertical, m_BlurTextureVertical;
		std::shared_ptr<Shader> m_BrightPassShader = ResourceManager::GetInstance().LoadResource<Shader>("BrightPassShader", std::make_tuple("fullScreen.vert", "BrightPassShader.frag", std::optional<std::string>{}));
		std::shared_ptr<Shader> m_BlurPassShader = ResourceManager::GetInstance().LoadResource<Shader>("BlurPassShader", std::make_tuple("fullScreen.vert", "BlurShader.frag", std::optional<std::string>{}));
		std::shared_ptr<Shader> m_CompositeShader = ResourceManager::GetInstance().LoadResource<Shader>("CompositeShader", std::make_tuple("fullScreen.vert", "BloomComposite.frag", std::optional<std::string>{}));
	};

}