#pragma once

#include <string>
#include <memory>
#include "REON/GameHierarchy/Components/Component.h"

namespace REON {

    class Scene;
    class Transform;

    class GameObject : public std::enable_shared_from_this<GameObject> {
    public:
        GameObject();
        ~GameObject();
        GameObject(const GameObject&);

        template <typename T>
        T* AddComponent(std::shared_ptr<T> component) {
            m_Components.emplace_back(component);
            m_Components.back()->SetOwner(shared_from_this());
            return dynamic_cast<T*>(m_Components.back().get());
        }

        template <typename T>
        T* GetComponent() {
            for (const auto& component : m_Components) {
                if (auto ptr = dynamic_cast<T*>(component.get())) {
                    return ptr;
                }
            }
            return nullptr;
        }


        void Update(float deltaTime);

        void AddChild(std::shared_ptr<GameObject>);

        std::shared_ptr<GameObject> GetParent();

        std::shared_ptr<Transform> GetTransform();

        std::shared_ptr<Scene> GetScene();

        void SetName(std::string newName);

        std::string GetName();

        void SetScene(std::shared_ptr<Scene> newScene);

        void OnGameObjectDeleted();

    public:
        bool enabled = true;

    private:
        void SetParent(std::shared_ptr<GameObject> newParent);

    private:
        std::vector<std::shared_ptr<Component>> m_Components;
        std::vector<std::shared_ptr<GameObject>> m_Children;
        std::weak_ptr<GameObject> m_Parent;
        std::string m_Name;

        std::shared_ptr<Transform> m_Transform;

        std::weak_ptr<Scene> m_Scene;
    };

}

