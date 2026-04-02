#pragma once
#include <REON/Platform/Vulkan/VulkanContext.h>
#include <REON/GameHierarchy/Components/Renderer.h>

namespace REON {

	class DirectionalShadowPass
	{
	public:
		DirectionalShadowPass() {}
		~DirectionalShadowPass() {}

		void Init(const VulkanContext* context);

		void render(const VulkanContext* context, std::vector<std::shared_ptr<Renderer>> renderers, glm::mat4 mainLightViewProj, VkSemaphore signalSemaphore);

		void createPerLightDescriptorSets(const VulkanContext* context);
		void createPerObjectDescriptorSets(const VulkanContext* context, std::shared_ptr<Renderer> renderer);

		std::vector<VkImageView> getShadowViews() const 
		{
            std::vector<VkImageView> views;
            for (auto& img : m_DepthImages)
            {
                views.push_back(img->getVkImageView());
			}
			return views;
		}
		VkSampler getShadowSampler() const { return m_DepthImageSampler; }

		void cleanup(const VulkanContext* context);

	private:
		void createCommandBuffers(const VulkanContext* context);
		void createImages(const VulkanContext* context);
		void createRenderPass(const VulkanContext* context);
		void createFrameBuffers(const VulkanContext* context);
		void createDescriptorSetLayout(const VulkanContext* context);
		void createGraphicsPipeline(const VulkanContext* context);
		void createPerLightBuffers(const VulkanContext* context);

		std::vector<VkCommandBuffer> m_CommandBuffers;
		VkCommandPool m_CommandPool;
		VkRenderPass m_RenderPass;
		VkDescriptorSetLayout m_PerLightDescriptorSetLayout;
		VkDescriptorSetLayout m_PerObjectDescriptorSetLayout;
		std::vector<VkDescriptorSet> m_PerLightDescriptorSets;
        std::vector<BufferHandle> m_PerLightBuffers;
		VkPipelineLayout m_PipelineLayout;
		VkPipeline m_GraphicsPipeline;
		std::vector<ImageHandle> m_DepthImages;
		std::vector<VkFramebuffer> m_Framebuffers;
		VkSampler m_DepthImageSampler;

		const uint MAIN_SHADOW_WIDTH = 4096, MAIN_SHADOW_HEIGHT = 4096;
	};

}