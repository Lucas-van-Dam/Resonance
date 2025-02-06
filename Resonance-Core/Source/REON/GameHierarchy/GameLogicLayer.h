#pragma once
#include "REON/Layer.h"

#include "REON/Core.h"

#include "REON/GameHierarchy/GameObject.h"

#include "REON/Events/KeyEvent.h"
#include "REON/Events/MouseEvent.h"
#include <REON/Events/EventBus.h>

namespace REON {

	class GameLogicLayer : public Layer
	{
	public:
		GameLogicLayer();
		~GameLogicLayer();

		void OnDetach() override;
		void OnAttach() override;
		void OnUpdate() override;
		void OnImGuiRender() override;

		void InitializeTestScene();

		void CheckKeyPressed();
		void OnKeyPressed(const KeyPressedEvent& event);

	private:
		float deltaTime = 0.0f;
		std::chrono::high_resolution_clock::time_point lastTime;
		CallbackID m_KeyPressedCallbackID;
	};

}

