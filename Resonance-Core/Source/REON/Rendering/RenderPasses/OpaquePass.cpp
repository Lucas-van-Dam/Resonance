#include "reonpch.h"
#include "OpaquePass.h"

namespace REON {
	void OpaquePass::init(const VulkanContext* context, uint width, uint height, std::vector<VkDescriptorSetLayout> layouts, VkPipelineCache cache)
	{
		m_Width = width;
		m_Height = height;
		m_Layouts = layouts;
		m_PipelineCache = cache;
		context->createCommandPool(m_CommandPool, context->findQueueFamilies(context->getPhysicalDevice()).graphicsFamily.value());
		createCommandBuffers(context);
		createImages(context);
		createBuffers(context);
		createRenderPasses(context);
		//createFrameBuffers(context);
		//createDescriptorSets(context);
		createGraphicsPipelines(context);
		createSyncObjects(context);
	}
}