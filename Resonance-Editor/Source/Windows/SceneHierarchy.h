#pragma once

#include <memory>
#include "Reon.h"
#include "imgui.h"


namespace REON::EDITOR {
	class SceneHierarchy
	{
	public:
		static void RenderGameObjectNode(std::shared_ptr<REON::GameObject> gameObject, std::shared_ptr<REON::GameObject>& selectedObject);

		static void ProcessReparenting();

		static void RenderSceneHierarchy(const std::vector<std::shared_ptr<REON::GameObject>>& rootObjects, std::shared_ptr<REON::GameObject>& selectedObject);

	private:
		static std::shared_ptr<REON::GameObject> pendingReparentSource;
		static std::shared_ptr<REON::GameObject> pendingReparentTarget;
	};
}

