#pragma once
#include "PostProcessingEffect.h"
#include <glad/glad.h>

namespace REON {

	class PostProcessingStack
	{
	public:
		void Init(int width, int height);
		void AddEffect(std::shared_ptr<PostProcessingEffect> pass);
		uint Render(uint inputTexture, uint depthTexture);
		void Resize(int width, int height);

	private:
		std::vector<std::shared_ptr<PostProcessingEffect>> m_Effects;
		uint m_PingPongFbo[2];
		uint m_PingPongTex[2];
		bool m_UsingPing;
	};

}