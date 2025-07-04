#pragma once
#include <REON/Platform/Vulkan/VulkanContext.h>
#include "REON/GameHierarchy/Components/Renderer.h"
#include "REON/GameHierarchy/Components/Camera.h"

namespace REON {
	struct CameraSwapchainRecources;

	struct CameraSwapchainRecourcesTransparent {
		VkFramebuffer framebuffer;
		VkFramebuffer compositeFramebuffer;

		VkBuffer frameInfoBuffer;
		VmaAllocation frameInfoAllocation;
		void* frameInfoMapped;
		VkDescriptorSet compositeDescriptorSet;

		VkImage colorAccumTarget;
		VmaAllocation colorAccumAllocation;
		VkImageView colorAccumView;
		VkImage alphaAccumTarget;
		VmaAllocation alphaAccumAllocation;
		VkImageView alphaAccumView;
		VkImage compositeTarget;
		VmaAllocation compositeAllocation;
		VkImageView compositeView;
	};

	struct FrameInfo {
		struct CameraFrameData {
			VkCommandBuffer commandBuffer;
			VkCommandBuffer compositeCommandBuffer;
		};

		std::unordered_map<std::shared_ptr<Camera>, CameraFrameData> cameraData;
	};

	class TransparentPass
	{
	public:
		TransparentPass() {}
		~TransparentPass() {}

		void init(const VulkanContext* context, std::vector<VkDescriptorSetLayout> layouts, VkPipelineCache cache);

		void init(const VulkanContext* context, std::shared_ptr<Camera> camera, std::vector<VkImageView>& resultViews,
			std::vector<VkImageView>& depthViews, std::vector<VkImageView>& opaqueViews);

		void render(const VulkanContext* context, std::shared_ptr<Camera> camera,
			std::unordered_map<std::string, std::unordered_map<std::string, std::vector<DrawCommand>>> rendererMap, 
			VkSemaphore waitSemaphore, VkSemaphore signalSemaphore, VkDescriptorSet globalDescriptorSet);

		void resize(const VulkanContext* context, std::shared_ptr<Camera> camera, std::vector<VkImageView>& endViews,
			std::vector<VkImageView> opaqueImageViews, std::vector<VkImageView> depthImageViews);

		void cleanup(const VulkanContext* context);

		void hotReloadShaders(const VulkanContext* context);

	private:
		void createImages(const VulkanContext* context, std::shared_ptr<Camera> camera);
		void createBuffers(const VulkanContext* context, std::shared_ptr<Camera> camera);
		void createRenderPasses(const VulkanContext* context);
		void createFrameBuffers(const VulkanContext* context, std::shared_ptr<Camera> camera, std::vector<VkImageView>& endImageViews, std::vector<VkImageView>& depthImageViews);
		void createGraphicsPipelines(const VulkanContext* context);
		void createDescriptorSetLayouts(const VulkanContext* context);
		void createDescriptorSets(const VulkanContext* context, std::shared_ptr<Camera> camera, std::vector<VkImageView> opaqueImageViews);
		VkPipeline createPermutationGraphicsPipeline(const VulkanContext* context, VkPipeline BasePipeline, uint32_t flags);
		VkPipeline getPipelineFromFlags(const VulkanContext* context, uint32_t flags);
		void cleanForResize(const VulkanContext* context, std::shared_ptr<Camera> camera);
		void createSyncObjects(const VulkanContext* context);

		//std::vector<VkCommandBuffer> m_CommandBuffers;
		VkCommandPool m_CommandPool;
		VkRenderPass m_RenderPass;

		std::unordered_map<std::shared_ptr<Camera>, std::vector<CameraSwapchainRecourcesTransparent>> m_SwapChainResourcesByCamera;
		std::vector<FrameInfo> m_FrameData;

		VkPipelineLayout m_GraphicsPipelineLayout;
		VkPipeline m_GraphicsPipeline;
		std::unordered_map<uint32_t, VkPipeline> m_PipelineByMaterialPermutation;

		//std::vector<VkCommandBuffer> m_CompositeCommandBuffers;
		VkRenderPass m_CompositeRenderPass;
		VkPipelineLayout m_CompositePipelineLayout;
		VkPipeline m_CompositePipeline;

		std::vector<VkSemaphore> m_TransparentRenderDone;

		std::vector<VkDescriptorSetLayout> m_Layouts;

		VkPipelineCache m_PipelineCache;

		VkDescriptorSetLayout m_CompositeDescriptorSetLayout;
	};

}