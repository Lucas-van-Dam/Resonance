#pragma once

#include "glm/glm.hpp"

#include "REON/GameHierarchy/Components/Component.h"
#include "nlohmann/json_fwd.hpp"

namespace REON
{

enum class LightType
{
    Spot = 0,
    Directional = 1,
    Ambient = 2,
    Point = 3
};

class [[clang::annotate("serialize")]] Light : public ComponentBase<Light>, public std::enable_shared_from_this<Light>
{
  public:
    explicit Light(LightType type = LightType::Point, float intensity = 1, glm::vec3 color = glm::vec3(1, 1, 1),
                   float innerCutOff = -1, float outerCutOff = -1);

    void update(float deltaTime) override;

    virtual void on_game_object_added_to_scene() override;

    void cleanup() override;

    virtual nlohmann::ordered_json serialize() const override;
    virtual void deserialize(const nlohmann::ordered_json& json, std::filesystem::path basePath) override;

  public:
    LightType type;
    float intensity;
    glm::vec3 color;
    float innerCutOff;
    float outerCutOff;

  private:
    virtual void on_component_detach() override;
};

} // namespace REON
