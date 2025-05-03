#pragma once
#include "PostProcessingEffect.h"
#include <glad/glad.h>

namespace REON {
	struct GPUTimerQuery {
		GLuint startQuery = 0;
		GLuint endQuery = 0;
	};

	class PostProcessingStack
	{
	public:
		void Init(int width, int height);
		void AddEffect(std::shared_ptr<PostProcessingEffect> pass);
		uint Render(uint inputTexture, uint depthTexture);
		void Resize(int width, int height);
		void HotReloadShaders();
		void ExportFrameDataToCSV(std::string path, std::string effectName);
		void StartProfiling();

	private:
		std::vector<std::shared_ptr<PostProcessingEffect>> m_Effects;
		uint m_PingPongFbo[2];
		uint m_PingPongTex[2];
		bool m_UsingPing;
		std::unordered_map<std::string, GPUTimerQuery> m_EffectTimers;
		std::unordered_map<std::string, std::vector<double>> m_EffectTimings;
		bool m_Profiling = false;
	};

}