#pragma once

#include "REON/GameHierarchy/Components/Light.h"

#define REON_MAX_LIGHTS 50

namespace REON {

    class LightManager : public std::enable_shared_from_this<LightManager> {
    public:
        LightManager(int maxLights);

        void AddLight(const std::shared_ptr<Light>& light);

    public:
        std::vector<std::shared_ptr<Light>> lights;
        int maxLights;
        std::shared_ptr<Light> mainLight;
    };

}

