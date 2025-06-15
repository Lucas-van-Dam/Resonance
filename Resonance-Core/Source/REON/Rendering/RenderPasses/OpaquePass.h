#pragma once
#include <REON/Platform/Vulkan/VulkanContext.h>
#include "REON/GameHierarchy/Components/Renderer.h"

namespace REON {
	class OpaquePass
	{
	public:
		void init(const VulkanContext* context, uint width, uint height, std::vector<VkDescriptorSetLayout> layouts, VkPipelineCache cache);

		void render(const VulkanContext* context,
			std::unordered_map<std::string, std::unordered_map<std::string, std::vector<DrawCommand>>> rendererMap,
			VkSemaphore waitSemaphore, VkSemaphore signalSemaphore, VkDescriptorSet globalDescriptorSet);

		void resize(const VulkanContext* context, uint width, uint height);

		void cleanup(const VulkanContext* context);

		void hotReloadShaders(const VulkanContext* context);

	private:
		void createCommandPool(const VulkanContext* context);
		void createCommandBuffers(const VulkanContext* context);
		void createImages(const VulkanContext* context);
		void createBuffers(const VulkanContext* context);
		void createRenderPasses(const VulkanContext* context);
		void createFrameBuffers(const VulkanContext* context, std::vector<VkImageView>& endImageViews, std::vector<VkImageView>& depthImageViews);
		void createGraphicsPipelines(const VulkanContext* context);
		void createDescriptorSets(const VulkanContext* context, std::vector<VkImageView> opaqueImageViews);
		VkPipeline createPermutationGraphicsPipeline(const VulkanContext* context, VkPipeline BasePipeline, uint32_t flags);
		VkPipeline getPipelineFromFlags(const VulkanContext* context, uint32_t flags);
		void cleanForResize(const VulkanContext* context);
		void createSyncObjects(const VulkanContext* context);

	private:
		uint m_Width, m_Height;
		std::vector<VkDescriptorSetLayout> m_Layouts;

		VkPipelineCache m_PipelineCache;

		VkCommandPool m_CommandPool;
	};

}