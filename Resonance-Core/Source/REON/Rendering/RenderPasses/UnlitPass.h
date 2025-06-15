#pragma once
#include <REON/Platform/Vulkan/VulkanContext.h>
#include <REON/Rendering/Material.h>
#include <REON/GameHierarchy/Components/Renderer.h>
#include <REON/ResourceManagement/ResourceManager.h>

namespace REON {
	enum RenderMode;

	class UnlitPass
	{
	public:
		UnlitPass() = default;
		~UnlitPass() = default;

		void init(const VulkanContext* context, uint width, uint height, std::vector<VkImageView> endImageViews, 
			VkPipelineCache pipelineCache, std::vector<VkDescriptorSetLayout> layouts);

		void render(const VulkanContext* context, 
			std::unordered_map<std::string, std::unordered_map<std::string, std::vector<DrawCommand>>> rendererMap,
			VkDescriptorSet globalDescriptorSet, VkSemaphore signalSemaphore, RenderMode renderMode);

		void resize(const VulkanContext* context, uint width, uint height, std::vector<VkImageView> endImageViews);

	private:
		void createImages(const VulkanContext* context);
		void createRenderPass(const VulkanContext* context);
		void createFrameBuffers(const VulkanContext* context, std::vector<VkImageView> endImageViews);
		void createDescriptorSetLayout(const VulkanContext* context);
		VkPipeline createGraphicsPipeline(const VulkanContext* context, VkPipelineCache pipelineCache, uint32_t flags);
		VkPipeline getPipelineFromFlags(const VulkanContext*, uint32_t flags);

	private:
		uint m_Width, m_Height;
		VkPipelineCache m_PipelineCache = VK_NULL_HANDLE;

		PFN_vkCmdSetPolygonModeEXT vkCmdSetPolygonModeEXT;

		std::unordered_map<uint32_t, VkPipeline> m_PipelineByMaterialPermutation;

		std::vector<VkCommandBuffer> m_CommandBuffers;
		VkCommandPool m_CommandPool;
		VkRenderPass m_RenderPass = VK_NULL_HANDLE;
		std::vector<VkFramebuffer> m_Framebuffers;
		VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;

		VkImage m_ColorImage = VK_NULL_HANDLE;
		VkImageView m_ColorImageView = VK_NULL_HANDLE;
		VmaAllocation m_ColorImageAllocation = VK_NULL_HANDLE;

		VkImage m_DepthImage = VK_NULL_HANDLE;
		VkImageView m_DepthImageView = VK_NULL_HANDLE;
		VmaAllocation m_DepthImageAllocation = VK_NULL_HANDLE;

		std::vector<VkDescriptorSetLayout> m_DescriptorSetLayouts;
	};

}