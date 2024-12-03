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

    class Light : public ComponentBase<Light> {
    public:
        explicit Light(LightType type = LightType::Spot, float intensity = 1, glm::vec3 color = glm::vec3(1, 1, 1), float innerCutOff = -1, float outerCutOff = -1);

        void Update(float deltaTime) override;

    public:
        LightType type;
        float intensity;
        glm::vec3 color;
        float innerCutOff;
        float outerCutOff;

    private:
        // Inherited via Component
        void OnGameObjectAddedToScene() override;

        void OnComponentDetach() override;

    };

}
