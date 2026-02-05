#pragma once
#include "Component.h"
#include "glm/ext/vector_uint2.hpp"
#include <glm/ext/matrix_float4x4.hpp>
#include <memory>

namespace REON
{
class [[clang::annotate("serialize")]] Camera : public ComponentBase<Camera>,
                                                public std::enable_shared_from_this<Camera>
{
  public:
    Camera() = default;
    void update(float deltaTime) override;
    void cleanup() override;
    virtual void on_game_object_added_to_scene() override;
    virtual void on_component_detach() override;

    virtual glm::mat4 GetViewMatrix() const
    {
        return m_ViewMatrix;
    }
    virtual glm::mat4 GetProjectionMatrix() const
    {
        return m_ProjectionMatrix;
    }

    virtual nlohmann::ordered_json serialize() const override;
    virtual void deserialize(const nlohmann::ordered_json& json, std::filesystem::path basePath) override;

  public:
    float fov = 60.0f;
    float nearPlane = 0.1f;
    float farPlane = 100.0f;

    bool primary = false;

    glm::uvec2 viewportSize = {800, 600};

  private:
    glm::mat4 m_ProjectionMatrix;
    glm::mat4 m_ViewMatrix;
};

} // namespace REON