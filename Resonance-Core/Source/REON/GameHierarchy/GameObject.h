#pragma once

#include <string>
#include <memory>
#include <vector>
#include "REON/GameHierarchy/Components/Component.h"
#include "REON/Object.h"

namespace REON {

    class Scene;
    class Transform;

    
    class [[clang::annotate("serialize")]] GameObject : public Object, public std::enable_shared_from_this<GameObject> {
    public:
        GameObject(const std::string& id = "");
        ~GameObject();
        GameObject(const GameObject&);

        template <typename T>
        T* AddComponent(std::shared_ptr<T> component) {
            m_Components.emplace_back(component);
            m_Components.back()->SetOwner(shared_from_this());
            if (GetScene())
                component->OnGameObjectAddedToScene();
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

        const std::vector<std::shared_ptr<Component>>& GetComponents() const;

        const std::vector<std::shared_ptr<GameObject>>& GetChildren() const;

        void Update(float deltaTime);

        void RemoveChild(std::shared_ptr<GameObject> child);

        void AddChild(std::shared_ptr<GameObject>);

        std::shared_ptr<GameObject> GetParent();

        std::shared_ptr<Transform> GetTransform();

        std::shared_ptr<Scene> GetScene();

        void SetScene(std::shared_ptr<Scene> newScene);

        void OnGameObjectDeleted();

        bool IsDescendantOf(std::shared_ptr<GameObject> other) const;

    public:
        bool enabled = true;

    private:
        void SetParent(std::shared_ptr<GameObject> newParent);

    private:
        std::vector<std::shared_ptr<Component>> m_Components;
        std::vector<std::shared_ptr<GameObject>> m_Children;
        std::weak_ptr<GameObject> m_Parent;

        std::shared_ptr<Transform> m_Transform;

        std::weak_ptr<Scene> m_Scene;

        template <typename ClassType, typename FieldType , FieldType ClassType::* field>
        friend struct ReflectionAccessor;
    };

}

