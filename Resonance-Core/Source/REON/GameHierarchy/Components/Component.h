#pragma once

#include "REON/Object.h"

#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>
#include <typeindex>

#include "nlohmann/json_fwd.hpp"


namespace REON
{

class GameObject;

class Component
{
  public:
    virtual ~Component() = default;
    virtual void update(float deltaTime) = 0;
    // virtual void Initialize() = 0;

    std::shared_ptr<GameObject> get_owner() const
    {
        return m_GameObject.lock();
    }

    virtual void set_owner(std::shared_ptr<GameObject> owner)
    {
        m_GameObject = owner;
    }

    virtual void cleanup() = 0;

    virtual void on_game_object_added_to_scene() = 0;
    virtual void on_component_detach() = 0;

    [[nodiscard]] virtual nlohmann::ordered_json serialize() const = 0;
    virtual void deserialize(const nlohmann::ordered_json& json, std::filesystem::path basePath) = 0;

    [[nodiscard]] virtual std::string get_type_name() const = 0;
    [[nodiscard]] virtual std::type_index get_type_index() const = 0;

  private:
    std::weak_ptr<GameObject> m_GameObject;
};

template <typename T> class ComponentBase : public Component, public Object
{
  public:
    ComponentBase() : Object(get_type_name()) {}
    std::string get_type_name() const override
    {
        std::string mangled_name = typeid(T).name();
        size_t pos = mangled_name.find_last_of("::");
        if (pos != std::string::npos)
        {
            return mangled_name.substr(pos + 1); // Skip "::"
        }
        return mangled_name;
    }
    [[nodiscard]] std::type_index get_type_index() const override
    {
        return typeid(T);
    }
};
} // namespace REON
