#include "reonpch.h"
#include "LightManager.h"
#include "REON/GameHierarchy/Scene.h"


namespace REON {

    LightManager::LightManager(int maxLights) : maxLights(maxLights) {

    }

    void LightManager::AddLight(const std::shared_ptr<Light>& light) {
        if (mainLight == nullptr && light->type == LightType::Directional) {
            mainLight = light;
            lights.insert(lights.begin(), light);
            light->GetOwner()->GetScene()->renderManager->setMainLight(light);
            return;
        }
        lights.emplace_back(light);
    }

}