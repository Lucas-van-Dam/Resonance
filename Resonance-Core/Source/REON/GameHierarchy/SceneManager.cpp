#include "reonpch.h"
#include "SceneManager.h"

namespace REON {
	SceneManager::SceneManager()
	{
		m_CurrentScene = std::make_shared<Scene>("TestScene");
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
}
