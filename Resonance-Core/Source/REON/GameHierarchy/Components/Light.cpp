#include "reonpch.h"

#include "Light.h"

#include "REON/GameHierarchy/GameObject.h"
#include "REON/GameHierarchy/Scene.h"

#include <nlohmann/json.hpp>


namespace REON
{

Light::Light(LightType type, float intensity, glm::vec3 color, float innerCutOff, float outerCutOff, float range)
    : type(type), intensity(intensity), color(color), innerCutOff(innerCutOff), outerCutOff(outerCutOff), range(range)
{
}

void Light::update(float deltaTime) {}

void Light::on_game_object_added_to_scene()
{
    get_owner()->GetScene()->lightManager->AddLight(shared_from_this());
}

void Light::cleanup() {}

void Light::on_component_detach() {}

} // namespace REON