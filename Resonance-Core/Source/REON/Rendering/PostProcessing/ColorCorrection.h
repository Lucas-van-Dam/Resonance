#pragma once
#include "PostProcessingEffect.h"
#include "REON/ResourceManagement/ResourceManager.h"

namespace REON {
	class ColorCorrection : public PostProcessingEffect
	{
	public:
		void Resize(int width, int height) override;
		void Apply(uint inputTexture, uint depthTexture, uint outputFbo) override;
		void Init(int width, int height) override;
		void HotReloadShaders() override;
		std::string GetName() const override;

	private:
		std::shared_ptr<Shader> m_ScreenShader = ResourceManager::GetInstance().LoadResource<Shader>("ScreenShader", std::make_tuple("fullScreen.vert", "fullScreen.frag", std::optional<std::string>{}));
	};

}