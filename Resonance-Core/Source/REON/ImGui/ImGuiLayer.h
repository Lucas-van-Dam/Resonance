#pragma once

#include "REON/Layer.h"
#include "REON/Events/ApplicationEvent.h"
#include "REON/Events/MouseEvent.h"
#include "REON/Events/KeyEvent.h"

#include "REON/Core.h"


namespace REON {

	class  ImGuiLayer : public Layer
	{
	public:
		ImGuiLayer();
		~ImGuiLayer();

		void OnDetach() override;
		void OnAttach() override;
		void OnImGuiRender() override;

		void Begin();
		void End();
	};

}