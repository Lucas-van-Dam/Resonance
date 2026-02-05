#include "reonpch.h"

#include "Light.h"

#include "REON/GameHierarchy/GameObject.h"
#include "REON/GameHierarchy/Scene.h"

#include <nlohmann/json.hpp>


namespace REON
{

Light::Light(LightType type, float intensity, glm::vec3 color, float innerCutOff, float outerCutOff)
    : type(type), intensity(intensity), color(color), innerCutOff(innerCutOff), outerCutOff(outerCutOff)
{
}

void Light::update(float deltaTime) {}

void Light::on_game_object_added_to_scene()
{
    get_owner()->GetScene()->lightManager->AddLight(shared_from_this());
}

void Light::cleanup() {}

nlohmann::ordered_json Light::serialize() const
{
    nlohmann::ordered_json json;

    json["Type"] = get_type_name();
    json["LightType"] = static_cast<int>(type);
    json["Intensity"] = intensity;
    json["Color"] = {color.r, color.g, color.b};
    json["InnerCutOff"] = innerCutOff;
    json["OuterCutOff"] = outerCutOff;

    return json;
}

void Light::deserialize(const nlohmann::ordered_json& json, std::filesystem::path basePath)
{
    if (json.contains("LightType"))
    {
        type = static_cast<LightType>(json["LightType"].get<int>());
    }
    if (json.contains("Intensity"))
    {
        intensity = json["Intensity"].get<float>();
    }
    if (json.contains("Color"))
    {
        auto colorArray = json["Color"];
        color = glm::vec3(colorArray[0], colorArray[1], colorArray[2]);
    }
    if (json.contains("InnerCutOff"))
    {
        innerCutOff = json["InnerCutOff"].get<float>();
    }
    if (json.contains("OuterCutOff"))
    {
        outerCutOff = json["OuterCutOff"].get<float>();
    }
}

void Light::on_component_detach() {}

} // namespace REON