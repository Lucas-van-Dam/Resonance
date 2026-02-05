#include "reonpch.h"

#include "Scene.h"

#include "REON/GameHierarchy/Components/Transform.h"
#include "REON/GameHierarchy/GameObject.h"


namespace REON
{

void Scene::AddGameObject(std::shared_ptr<GameObject> gameObject)
{
    m_GameObjectsToAdd.push_back(gameObject);
    gameObject->SetScene(shared_from_this());
}

void Scene::DeleteGameObject(std::shared_ptr<GameObject> gameObject)
{
    m_GameObjectsToDelete.push_back(gameObject);
}

void Scene::UpdateScene(float deltaTime)
{
    for (const auto& gameObject : m_GameObjects)
    {
        gameObject->update(deltaTime);
    }
}

void Scene::ProcessGameObjectAddingAndDeletion()
{
    for (const auto gameObject : m_GameObjectsToAdd)
    {
        if (gameObject)
        {
            m_GameObjects.push_back(gameObject);
        }
    }
    for (const auto& gameObject : m_GameObjectsToDelete)
    {
        auto obj = gameObject.lock();
        if (obj)
        {
            obj->OnGameObjectDeleted();
            m_GameObjects.erase(std::remove(m_GameObjects.begin(), m_GameObjects.end(), obj), m_GameObjects.end());
        }
    }
    m_GameObjectsToAdd.clear();
    m_GameObjectsToDelete.clear();
}

void Scene::InitializeSceneWithObjects()
{
    std::shared_ptr<GameObject> MainLight = std::make_shared<GameObject>();
    std::shared_ptr<Light> lightComponent = std::make_shared<Light>(LightType::Directional, 3, glm::vec3(1, 1, 1));
    AddGameObject(MainLight);
    MainLight->AddComponent<Light>(lightComponent);
    MainLight->GetTransform()->localRotation.setFromEulerAngles(110, 0, 0);
    MainLight->SetName("MainLight");
}

std::shared_ptr<EditorCamera> Scene::GetEditorCamera()
{
    return std::dynamic_pointer_cast<EditorCamera>(cameras[0]);
}

Scene::Scene(const std::string& name) : Object(name)
{
    cameras.push_back(EditorCamera::GetInstance());
    lightManager = std::make_unique<LightManager>(5);
    renderManager = std::make_unique<RenderManager>(lightManager, std::dynamic_pointer_cast<EditorCamera>(cameras[0]));
}

Scene::~Scene()
{
    m_GameObjects.clear();
    // renderManager->cleanup();
    // renderManager.release();
}

std::shared_ptr<GameObject> Scene::GetGameObject(int index)
{
    if (index >= 0 && index < static_cast<int>(m_GameObjects.size()))
    {
        auto gameObject = m_GameObjects[index];
        if (gameObject)
        {
            return gameObject;
        }
    }
    // Return nullptr if the index is out of bounds or the GameObject is deleted
    return nullptr;
}

std::vector<std::shared_ptr<GameObject>> Scene::GetRootObjects()
{
    return m_GameObjects;
}

} // namespace REON