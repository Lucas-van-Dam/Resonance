#pragma once

#include "ResourceManagement/Resource.h"

#include "Rendering/Shader.h"

namespace REON
{
struct EngineDefaults
{
    ResourceHandle<Shader> defaultLit;

    void Init() {

    }
};
}