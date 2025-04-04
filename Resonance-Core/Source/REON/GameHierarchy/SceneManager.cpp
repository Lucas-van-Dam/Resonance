#include "reonpch.h"
#include "SceneManager.h"

namespace REON {
	SceneManager::SceneManager()
	{

	}

	SceneManager::~SceneManager()
	{
	}

	std::shared_ptr<Scene> SceneManager::GetCurrentScene()
	{
		return m_CurrentScene;
	}

	std::shared_ptr<SceneManager> SceneManager::Get()
	{
		static std::shared_ptr<SceneManager> instance = std::make_shared<SceneManager>();
		return instance;
	}

	void SceneManager::SetActiveScene(std::shared_ptr<Scene> scene)
	{
		m_CurrentScene = scene;
	}
}