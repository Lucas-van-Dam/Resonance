#pragma once
#include "PostProcessingEffect.h"
#include "REON/ResourceManagement/ResourceManager.h"

namespace REON {
	class DepthOfField : public PostProcessingEffect {
	public:
		void Resize(int width, int height) override;
		void Apply(uint inputTexture, uint depthTexture, uint outputFbo) override;
		void Init(int width, int height) override;
		void HotReloadShaders() override;
		std::string GetName() const override;

		static float m_FocusDistance;
		static float m_FocusRange;

	private:

		uint m_CoCFbo, m_CoCTexture;
		uint m_BlurFbo, m_BlurTexture;

		std::shared_ptr<Shader> m_CoCShader = ResourceManager::GetInstance().LoadResource<Shader>("CoCShader", std::make_tuple("fullScreen.vert", "DepthOfField/CoC.frag", std::optional<std::string>{}));
		std::shared_ptr<Shader> m_BlurShader = ResourceManager::GetInstance().LoadResource<Shader>("DoFBlurShader", std::make_tuple("fullScreen.vert", "DepthOfField/Blur.frag", std::optional<std::string>{}));
		std::shared_ptr<Shader> m_CompositeShader = ResourceManager::GetInstance().LoadResource<Shader>("DoFCompositeShader", std::make_tuple("fullScreen.vert", "DepthOfField/Composite.frag", std::optional<std::string>{}));
	};

}

