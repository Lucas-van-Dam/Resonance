#pragma once

#include <memory>
#include <vector>
#include "REON/GameHierarchy/GameObject.h"
#include "REON/EditorCamera.h"
#include "REON/Rendering/LightManager.h"
#include "REON/Rendering/RenderManager.h"

namespace REON {

    class Scene : public Object, public std::enable_shared_from_this<Scene> {
    public:
        Scene(const std::string& name);
        Scene() {}
        ~Scene();
        void AddGameObject(std::shared_ptr<GameObject> gameObject);
        void DeleteGameObject(std::shared_ptr<GameObject> gameObject);
        void UpdateScene(float deltaTime);
        void ProcessGameObjectDeletion();
        std::shared_ptr<GameObject> GetGameObject(int index);
        std::vector<std::shared_ptr<GameObject>> GetRootObjects();
        std::shared_ptr<EditorCamera> GetEditorCamera();

    public:
        std::shared_ptr<EditorCamera> camera;
        std::shared_ptr<LightManager> lightManager;
        std::unique_ptr<RenderManager> renderManager;
        std::shared_ptr<GameObject> selectedObject;

    private:
        std::vector<std::shared_ptr<GameObject>> m_GameObjects;
        std::vector<std::weak_ptr<GameObject>> m_GameObjectsToDelete;
    };

}

