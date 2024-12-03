#pragma once

#include "REON/Layer.h"

#include "REON/Core.h"
#include "REON/Events/KeyEvent.h"
#include "REON/Events/MouseEvent.h"

namespace REON {

	class RenderLayer : public Layer
	{
	public:
		RenderLayer() : Layer("RenderLayer") {}
		~RenderLayer() {}

		//void OnDetach() override;
		//void OnAttach() override;
		void OnUpdate() override;
		//void OnImGuiRender() override;
		//void OnEvent(Event& event) override;

	private:
	};

}