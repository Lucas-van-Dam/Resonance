#pragma once

#include <memory>
#include <typeindex>

#include "REON/Object.h"

namespace REON {

    class GameObject;

    class Component {
    public:
        virtual ~Component() = default;
        virtual void Update(float deltaTime) = 0;
        //virtual void Initialize() = 0;
        
        std::shared_ptr<GameObject> GetOwner() const {
            return m_GameObject.lock();
        }

        virtual void SetOwner(std::shared_ptr<GameObject> owner) {
            m_GameObject = owner;
        }

        virtual void cleanup() = 0;

        virtual void OnGameObjectAddedToScene() = 0;
        virtual void OnComponentDetach() = 0;

        virtual std::string GetTypeName() const = 0;
        virtual std::type_index GetTypeIndex() const = 0;
    private:
        std::weak_ptr<GameObject> m_GameObject;
    };

    template <typename T>
    class ComponentBase : public Component, public Object {
    public:
        ComponentBase() : Object(GetTypeName()) {}
        std::string GetTypeName() const override 
        { 
            std::string mangledName = typeid(T).name();
            size_t pos = mangledName.find_last_of("::");
            if (pos != std::string::npos) {
                return mangledName.substr(pos + 1); // Skip "::"
            }
            return mangledName;
        }
        std::type_index GetTypeIndex() const override { return typeid(T); }
    };
}

