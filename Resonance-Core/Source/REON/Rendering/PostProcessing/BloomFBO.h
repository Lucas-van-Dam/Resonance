#pragma once
#include "glm/glm.hpp"

namespace REON {
	struct BloomMip {
		glm::vec2 size;
		glm::ivec2 intSize;
		uint texture;
	};

	class BloomFBO
	{
	public:
		BloomFBO();
		~BloomFBO();
		bool Init(uint windowWidth, uint windowHeight, uint mipChainLength);
		void Destroy();
		void BindForWriting();
		const std::vector<BloomMip>& MipChain() const;
		void Resize(uint windowWidth, uint windowHeight);

		uint m_FBO;

	private:
		bool m_Init;
		std::vector<BloomMip> m_MipChain;
	};

}