#include "reonpch.h"
#include "Scene.h"

namespace REON {

    void Scene::AddGameObject(std::shared_ptr<GameObject> gameObject) {
        m_GameObjectsToAdd.push_back(gameObject);
        gameObject->SetScene(shared_from_this());
    }

    void Scene::DeleteGameObject(std::shared_ptr<GameObject> gameObject)
    {
        m_GameObjectsToDelete.push_back(gameObject);
    }

    void Scene::UpdateScene(float deltaTime) {
        for (const auto& gameObject : m_GameObjects) {
            gameObject->Update(deltaTime);
        }
    }

    void Scene::ProcessGameObjectAddingAndDeletion() {
        for (const auto gameObject : m_GameObjectsToAdd) {
            if (gameObject) {
                m_GameObjects.push_back(gameObject);
            }
        }
        for (const auto& gameObject : m_GameObjectsToDelete) {
            auto obj = gameObject.lock();
            if (obj) {
                obj->OnGameObjectDeleted();
                m_GameObjects.erase(std::remove(m_GameObjects.begin(), m_GameObjects.end(), obj), m_GameObjects.end());
            }
        }
        m_GameObjectsToAdd.clear();
        m_GameObjectsToDelete.clear();
    }


    std::shared_ptr<EditorCamera> Scene::GetEditorCamera() {
        return camera;
    }

    Scene::Scene(const std::string& name) : Object(name) {
        this->camera = EditorCamera::GetInstance();
        lightManager = std::make_unique<LightManager>(5);
        renderManager = std::make_unique<RenderManager>(lightManager, camera);
    }

    Scene::~Scene() {
        m_GameObjects.clear();
    }

    std::shared_ptr<GameObject> Scene::GetGameObject(int index)
    {
        if (index >= 0 && index < static_cast<int>(m_GameObjects.size())) {
            auto gameObject = m_GameObjects[index];
            if (gameObject) {
                return gameObject;
            }
        }
        // Return nullptr if the index is out of bounds or the GameObject is deleted
        return nullptr;
    }

    std::vector<std::shared_ptr<GameObject>> Scene::GetRootObjects() {
        return m_GameObjects;
    }

}