#include "reonpch.h"
#include "RenderLayer.h"
#include "RenderManager.h"
#include "REON/GameHierarchy/SceneManager.h"
#include "REON/KeyCodes.h"
#include <imgui.h>
#include <GLFW/glfw3.h>
#include "REON/Application.h"
#include "REON/MouseButtonCodes.h"
#include "ImGuizmo.h"
#include <glm/gtc/type_ptr.hpp>
#include "REON/GameHierarchy/Components/Transform.h"

namespace REON {

	void RenderLayer::OnUpdate()
	{
		if (auto scene = SceneManager::Get()->GetCurrentScene()) {
			scene->renderManager->preRender();
			scene->renderManager->Render(scene->GetEditorCamera());
			if (scene->cameras.size() > 1) {
				scene->renderManager->Render(scene->cameras[1]);
			}
		}
	}
	void RenderLayer::OnCleanup()
	{
		if (auto scene = SceneManager::Get()->GetCurrentScene())
			scene->renderManager->cleanup();
	}
}