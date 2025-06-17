#pragma once
#include "REON/Layer.h"
#include "AudioManager.h"

namespace REON::AUDIO {
	class AudioLayer : public Layer
	{
	public:
		AudioLayer() : Layer("AudioLayer") {};

		void OnDetach() override;
		void OnAttach() override;
		void OnUpdate() override;
		void OnImGuiRender() override;

	private:
		AudioManager manager;
	};

}