#pragma once
#include <REON/Platform/Vulkan/VulkanContext.h>
#include "REON/GameHierarchy/Components/Renderer.h"

namespace REON {
	class TransparentPass
	{
	public:
		TransparentPass() {}
		~TransparentPass() {}

		void init(const VulkanContext* context, uint width, uint height, std::vector<VkDescriptorSetLayout> layouts, VkPipelineCache cache,
			std::vector<VkImageView> opaqueViews, std::vector<VkImageView>& resultViews, std::vector<VkImageView> depthImageViews);

		void render(const VulkanContext* context, 
			std::unordered_map<std::string, std::unordered_map<std::string, std::vector<DrawCommand>>> rendererMap, 
			VkSemaphore waitSemaphore, VkSemaphore signalSemaphore, VkDescriptorSet globalDescriptorSet);

		void resize(const VulkanContext* context, uint width, uint height, std::vector<VkImageView>& endViews, 
			std::vector<VkImageView> opaqueImageViews, std::vector<VkImageView> depthImageViews);

		void cleanup(const VulkanContext* context);

		void hotReloadShaders(const VulkanContext* context);

	private:
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

		std::vector<VkCommandBuffer> m_CommandBuffers;
		VkCommandPool m_CommandPool;
		VkRenderPass m_RenderPass;
		std::vector<VkFramebuffer> m_Framebuffers;

		VkPipelineLayout m_GraphicsPipelineLayout;
		VkPipeline m_GraphicsPipeline;
		std::unordered_map<uint32_t, VkPipeline> m_PipelineByMaterialPermutation;

		std::vector<VkCommandBuffer> m_CompositeCommandBuffers;
		VkRenderPass m_CompositeRenderPass;
		VkPipelineLayout m_CompositePipelineLayout;
		VkPipeline m_CompositePipeline;
		std::vector<VkFramebuffer> m_CompositeFramebuffers;

		std::vector<VkSemaphore> m_TransparentRenderDone;

		uint m_Width, m_Height;
		std::vector<VkDescriptorSetLayout> m_Layouts;

		VkPipelineCache m_PipelineCache;

		VkDescriptorSetLayout m_CompositeDescriptorSetLayout;
		std::vector<VkDescriptorSet> m_CompositeDescriptorSets;

		std::vector<VkBuffer> m_FrameInfoBuffers;
		std::vector<VmaAllocation> m_FrameInfoAllocations;
		std::vector<void*> m_FrameInfoBuffersMapped;


		//std::vector<VkDescriptorSet> m_GlobalDescriptorSets;
		//std::vector<VkBuffer> m_GlobalDataBuffers;
		//std::vector<VmaAllocation> m_GlobalDataBufferAllocations;
		//std::vector<void*> m_GlobalDataBuffersMapped;

		//std::vector<VkDescriptorSet> m_LightDescriptorSets;
		//std::vector<VkBuffer> m_LightDataBuffers;
		//std::vector<VmaAllocation> m_LightDataBufferAllocations;
		//std::vector<void*> m_LightDataBuffersMapped;

		std::vector<VkImage> m_ColorAccumTargets;
		std::vector<VmaAllocation> m_ColorAccumAllocations;
		std::vector<VkImageView> m_ColorAccumViews;
		std::vector<VkImage> m_AlphaAccumTargets;
		std::vector<VmaAllocation> m_AlphaAccumAllocations;
		std::vector<VkImageView> m_AlphaAccumViews;

		std::vector<VkImage> m_CompositeTargets;
		std::vector<VmaAllocation> m_CompositeAllocations;
		std::vector<VkImageView> m_CompositeViews;
	};

}