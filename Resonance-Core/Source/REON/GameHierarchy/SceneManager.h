#pragma once
#include "Scene.h"

namespace REON {
	class SceneManager
	{
	public:
		SceneManager();
		~SceneManager();

		static std::shared_ptr<SceneManager> Get();

		void SetActiveScene(std::shared_ptr<Scene> scene);

		std::shared_ptr<Scene> GetCurrentScene();

	private:
		std::shared_ptr<Scene> m_CurrentScene;
	};
}

