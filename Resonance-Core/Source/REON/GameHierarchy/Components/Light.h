#pragma once

#include "glm/glm.hpp"

#include "REON/GameHierarchy/Components/Component.h"

namespace REON {

    enum class LightType {
        Spot = 0,
        Directional = 1,
        Ambient = 2,
        Point = 3
    };

    
    class [[clang::annotate("serialize")]] Light : public ComponentBase<Light>, public std::enable_shared_from_this<Light> {
    public:
        explicit Light(LightType type = LightType::Point, float intensity = 1, glm::vec3 color = glm::vec3(1, 1, 1), float innerCutOff = -1, float outerCutOff = -1);

        void Update(float deltaTime) override;

        void OnGameObjectAddedToScene() override;

        void cleanup() override;

        nlohmann::ordered_json Serialize() const override;
        void Deserialize(const nlohmann::ordered_json& json, std::filesystem::path basePath) override;

    public:
        LightType type;
        float intensity;
        glm::vec3 color;
        float innerCutOff;
        float outerCutOff;

    private:
        void OnComponentDetach() override;
    };

}
