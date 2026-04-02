#pragma once
#include "REON/GameHierarchy/Components/Transform.h"

#include <REON/GameHierarchy/Components/Light.h>
#include <REON/GameHierarchy/Components/Renderer.h>
#include <memory>

namespace REON_EDITOR
{
class ComponentDrawers
{
  public:
    static void DrawInspector_Transform(std::shared_ptr<REON::Transform> transform);
    static void DrawInspector_Light(std::shared_ptr<REON::Light> light);
    static void DrawInspector_Renderer(std::shared_ptr<REON::Renderer> renderer);
};

} // namespace REON_EDITOR