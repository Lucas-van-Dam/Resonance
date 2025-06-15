#pragma once
#include <memory>
#include "REON/GameHierarchy/Components/Transform.h"
#include <REON/GameHierarchy/Components/Light.h>
#include <REON/GameHierarchy/Components/Renderer.h>

namespace REON::EDITOR {
	class ComponentDrawers
	{
	public:
		static void DrawInspector_Transform(std::shared_ptr<Transform> transform);
		static void DrawInspector_Light(std::shared_ptr<Light> light);
		static void DrawInspector_Renderer(std::shared_ptr<Renderer> renderer);
	};

}