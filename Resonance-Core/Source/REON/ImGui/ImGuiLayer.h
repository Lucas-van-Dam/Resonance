#pragma once

#include "REON/Layer.h"
#include "REON/Events/ApplicationEvent.h"
#include "REON/Events/MouseEvent.h"
#include "REON/Events/KeyEvent.h"

#include "REON/Core.h"
#include <REON/Platform/Vulkan/VulkanContext.h>

namespace REON {

	class  ImGuiLayer : public Layer
	{
	public:
		ImGuiLayer(VulkanContext* context);
		~ImGuiLayer();

		void OnDetach() override;
		void OnAttach() override;
		void OnImGuiRender() override;
		void OnCleanup() override;

		void createFrameBuffers();
		void onResize(const ContextResizeEvent& event);

		void Begin();
		void End();

	private:
		void SetupImGuiStyle();

		VulkanContext* m_Context; // TODO: CHANGE LATER TO BE ABSTRACT;
		VkRenderPass m_RenderPass;
		std::vector<VkCommandBuffer> m_CommandBuffers;
		VkCommandPool m_CommandPool;
		std::vector<VkFramebuffer> m_Framebuffers;
	};

}