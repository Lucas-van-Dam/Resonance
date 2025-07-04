#include "RuntimeLayer.h"
#include <REON/Logger.h>
#include <imgui/imgui.h>
#include <Reon.h>

namespace REON::RUNTIME {
	void RuntimeLayer::OnAttach()
	{
		REON_INFO("RuntimeLayer attached");

		//load scene from a config file or similar

	}

	void RuntimeLayer::OnImGuiRender()
	{
		auto size = ImGui::GetContentRegionAvail();
		if (auto scene = SceneManager::Get()->GetCurrentScene()) {
			ImGui::Image((ImTextureID)scene->renderManager->GetEndBuffer(scene->GetEditorCamera()), size, ImVec2(0, 1), ImVec2(1, 0));
		}
	}
}