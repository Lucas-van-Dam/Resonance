#pragma once
#include "PostProcessingEffect.h"
#include "REON/ResourceManagement/ResourceManager.h"
#include "BloomFBO.h"

namespace REON {
	class BloomEffect : public PostProcessingEffect
	{
	public:
		BloomEffect();
		~BloomEffect();
		void Init(int width, int height) override;
		void Destroy();
		void RenderBloomTexture(unsigned int srcTexture, float filterRadius);
		unsigned int BloomTexture();
		void Resize(int width, int height) override;
		void Apply(uint inputTexture, uint depthTexture, uint outputFbo) override;
		void HotReloadShaders() override;


		static float FilterRadius;
		static float bloomStrength;
		static float bloomThreshold;

	private:
		void RenderDownsamples(unsigned int srcTexture);
		void RenderUpsamples(float filterRadius);

		float GetWeightForMip(int mipLevel);

		bool m_Init;
		BloomFBO m_FBO;
		glm::ivec2 m_SrcViewportSize;
		glm::vec2 m_SrcViewportSizeFloat;
		std::shared_ptr<Shader> m_DownsampleShader = ResourceManager::GetInstance().LoadResource<Shader>("DownsampleShader", std::make_tuple("fullScreen.vert", "Bloom/DownSample.frag", std::optional<std::string>{}));
		std::shared_ptr<Shader> m_UpsampleShader = ResourceManager::GetInstance().LoadResource<Shader>("UpsampleShader", std::make_tuple("fullScreen.vert", "Bloom/UpSample.frag", std::optional<std::string>{}));
		std::shared_ptr<Shader> m_CompositeShader = ResourceManager::GetInstance().LoadResource<Shader>("CompositeShader", std::make_tuple("fullScreen.vert", "Bloom/Composite.frag", std::optional<std::string>{}));
	};

}