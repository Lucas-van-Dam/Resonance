#pragma once
#include <string>
#include "REON/Layer.h"

namespace REON::RUNTIME {
	class RuntimeLayer : public Layer
	{
	public:
		RuntimeLayer() : Layer("RuntimeLayer") {}
		~RuntimeLayer() override = default;
		void OnAttach() override;
		void OnDetach() override {}
		void OnUpdate() override {}
		void OnImGuiRender() override;
	};

}