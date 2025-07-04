#include "reonpch.h"
#include "TransparentPass.h"
#include "REON/Rendering/Material.h"
#include "REON/Rendering/Structs/Vertex.h"
#include "REON/ResourceManagement/ResourceManager.h"
#include "OpaquePass.h"

namespace REON {
	void TransparentPass::init(const VulkanContext* context,
		std::vector<VkDescriptorSetLayout> layouts, VkPipelineCache cache)
	{
		m_FrameData.resize(context->MAX_FRAMES_IN_FLIGHT);
		m_Layouts = layouts;
		m_PipelineCache = cache;
		context->createCommandPool(m_CommandPool, context->findQueueFamilies(context->getPhysicalDevice()).graphicsFamily.value());
		createDescriptorSetLayouts(context);
		createRenderPasses(context);
		createGraphicsPipelines(context);
		createSyncObjects(context);
	}

	void TransparentPass::init(const VulkanContext* context, std::shared_ptr<Camera> camera,
		std::vector<VkImageView>& resultViews, std::vector<VkImageView>& depthViews,
		std::vector<VkImageView>& opaqueViews)
	{
		std::vector<VkCommandBuffer> commandBuffers(context->MAX_FRAMES_IN_FLIGHT);
		std::vector<VkCommandBuffer> compositeCommandBuffers(context->MAX_FRAMES_IN_FLIGHT);
		context->createCommandBuffers(commandBuffers, m_CommandPool, context->MAX_FRAMES_IN_FLIGHT);
		context->createCommandBuffers(compositeCommandBuffers, m_CommandPool, context->MAX_FRAMES_IN_FLIGHT);
		for (int i = 0; i < context->MAX_FRAMES_IN_FLIGHT; i++) {
			m_FrameData[i].cameraData[camera].commandBuffer = commandBuffers[i];
			m_FrameData[i].cameraData[camera].compositeCommandBuffer = compositeCommandBuffers[i];
		}
		createImages(context, camera);
		createBuffers(context, camera);
		createFrameBuffers(context, camera, resultViews, depthViews);
		createDescriptorSets(context, camera, opaqueViews);
	}

	void TransparentPass::render(const VulkanContext* context, std::shared_ptr<Camera> camera,
		std::unordered_map<std::string, std::unordered_map<std::string, std::vector<DrawCommand>>> rendererMap,
		VkSemaphore waitSemaphore, VkSemaphore signalSemaphore, VkDescriptorSet globalDescriptorSet)
	{
		int currentFrame = context->getCurrentFrame();
		int currentImageIndex = context->getCurrentImageIndex();

		auto& swapChainResources = m_SwapChainResourcesByCamera[camera][currentImageIndex];
		auto& cameraData = m_FrameData[currentFrame].cameraData[camera];

		for (const auto& pair : rendererMap) {
			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = 0;
			beginInfo.pInheritanceInfo = nullptr;

			VkResult res = vkBeginCommandBuffer(cameraData.commandBuffer, &beginInfo);
			REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to begin recording command buffer");


			VkRenderPassBeginInfo renderPassInfo{};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = m_RenderPass;
			renderPassInfo.framebuffer = swapChainResources.framebuffer;
			renderPassInfo.renderArea.offset = { 0,0 };
			renderPassInfo.renderArea.extent = { camera->viewportSize.x, camera->viewportSize.y};

			std::array<VkClearValue, 3> clearValues{};
			clearValues[0].color = { 0.0f, 0.0f, 0.0f, 0.0f };
			clearValues[1].color = { 1.0f, 0.0f, 0.0f, 0.0f };
			clearValues[1].depthStencil = { 0.0f, 0 };
			clearValues[2].depthStencil = { 0.0f, 0 };
			renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
			renderPassInfo.pClearValues = clearValues.data();

			vkCmdBeginRenderPass(cameraData.commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);


			const auto& materialFromShader = pair.second;
			for (const auto& material : materialFromShader) {

				//set material wide buffers/textures
				std::shared_ptr<Material> mat = ResourceManager::GetInstance().GetResource<Material>(material.first);
				if (!(mat->blendingMode == Blend && mat->renderingMode == Transparent)) {
					continue;
				}
				auto pipeline = getPipelineFromFlags(context, mat->materialFlags);
				if (pipeline == VK_NULL_HANDLE) {
					REON_CORE_WARN("Cant render because pipeline is not found");
					continue;
				}
				vkCmdBindPipeline(cameraData.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

				VkViewport viewport{};
				viewport.x = 0.0f;
				viewport.y = 0.0f;
				viewport.width = static_cast<float>(camera->viewportSize.x);
				viewport.height = static_cast<float>(camera->viewportSize.y);
				viewport.minDepth = 0.0f;
				viewport.maxDepth = 1.0f;
				vkCmdSetViewport(cameraData.commandBuffer, 0, 1, &viewport);

				VkRect2D scissor{};
				scissor.offset = { 0,0 };
				scissor.extent = { camera->viewportSize.x, camera->viewportSize.y };
				vkCmdSetScissor(cameraData.commandBuffer, 0, 1, &scissor);

				memcpy(mat->flatDataBuffersMapped[currentFrame], &mat->flatData, sizeof(mat->flatData));

				vkCmdSetCullMode(cameraData.commandBuffer, mat->getDoubleSided() ? VK_CULL_MODE_NONE : VK_CULL_MODE_BACK_BIT);


				for (const auto& cmd : material.second) {
					std::vector<VkDescriptorSet> descriptorSets = { globalDescriptorSet, mat->descriptorSets[currentFrame], cmd.owner->objectDescriptorSets[context->getCurrentFrame()] };

					ObjectRenderData data{};
					data.model = cmd.owner->getModelMatrix();
					data.transposeInverseModel = glm::transpose(glm::inverse(data.model));
					memcpy(cmd.owner->objectDataBuffersMapped[context->getCurrentFrame()], &data, sizeof(data));

					VkBuffer vertexBuffers[] = { cmd.mesh->m_VertexBuffer };
					VkDeviceSize offsets[] = { 0 };
					vkCmdBindVertexBuffers(cameraData.commandBuffer, 0, 1, vertexBuffers, offsets);

					vkCmdBindIndexBuffer(cameraData.commandBuffer, cmd.mesh->m_IndexBuffer, 0, VK_INDEX_TYPE_UINT32);

					vkCmdBindDescriptorSets(cameraData.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipelineLayout, 0, descriptorSets.size(), descriptorSets.data(), 0, nullptr);
					vkCmdDrawIndexed(cameraData.commandBuffer, static_cast<uint32_t>(cmd.indexCount), 1, cmd.startIndex, 0, 0);
				}
			}



			vkCmdEndRenderPass(cameraData.commandBuffer);

			res = vkEndCommandBuffer(cameraData.commandBuffer);
			REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to record command buffer");

			VkSubmitInfo submitInfo{};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

			VkSemaphore waitSemaphores[] = { waitSemaphore };
			VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
			submitInfo.waitSemaphoreCount = 1;
			submitInfo.pWaitSemaphores = waitSemaphores;
			submitInfo.pWaitDstStageMask = waitStages;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &cameraData.commandBuffer;

			VkSemaphore signalSemaphores[] = { m_TransparentRenderDone[currentFrame] };
			submitInfo.signalSemaphoreCount = 1;
			submitInfo.pSignalSemaphores = signalSemaphores;

			res = vkQueueSubmit(context->getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
			REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to submit draw command buffer");
		}

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0;
		beginInfo.pInheritanceInfo = nullptr;

		VkResult res = vkBeginCommandBuffer(cameraData.compositeCommandBuffer, &beginInfo);
		REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to begin recording command buffer");

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_CompositeRenderPass;
		renderPassInfo.framebuffer = swapChainResources.compositeFramebuffer;
		renderPassInfo.renderArea.offset = { 0,0 };
		renderPassInfo.renderArea.extent = { camera->viewportSize.x, camera->viewportSize.y };

		std::array<VkClearValue, 1> clearValues{};
		clearValues[0].color = { 0.0f, 0.0f, 0.0f, 0.0f };
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(cameraData.compositeCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);



		vkCmdBindPipeline(cameraData.compositeCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_CompositePipeline);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(camera->viewportSize.x);
		viewport.height = static_cast<float>(camera->viewportSize.y);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(cameraData.compositeCommandBuffer, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = { 0,0 };
		scissor.extent = { camera->viewportSize.x, camera->viewportSize.y };
		vkCmdSetScissor(cameraData.compositeCommandBuffer, 0, 1, &scissor);

		glm::vec2 frameBufferSize(camera->viewportSize.x, camera->viewportSize.y);
		memcpy(swapChainResources.frameInfoMapped, &frameBufferSize, sizeof(glm::vec2));

		vkCmdBindDescriptorSets(cameraData.compositeCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_CompositePipelineLayout, 0, 1, &swapChainResources.compositeDescriptorSet, 0, nullptr);
		vkCmdDraw(cameraData.compositeCommandBuffer, 3, 1, 0, 0);



		vkCmdEndRenderPass(cameraData.compositeCommandBuffer);

		res = vkEndCommandBuffer(cameraData.compositeCommandBuffer);
		REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to record command buffer");

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { m_TransparentRenderDone[currentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cameraData.compositeCommandBuffer;

		VkSemaphore signalSemaphores[] = { signalSemaphore };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		res = vkQueueSubmit(context->getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
		REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to submit draw command buffer");
	}

	void TransparentPass::resize(const VulkanContext* context, std::shared_ptr<Camera> camera, std::vector<VkImageView>& endViews,
		std::vector<VkImageView> opaqueImageViews, std::vector<VkImageView> depthImageViews)
	{
		cleanForResize(context, camera);

		createImages(context, camera);
		createFrameBuffers(context, camera, endViews, depthImageViews);

		for (size_t i = 0; i < context->getAmountOfSwapChainImages(); i++) {
			VkDescriptorImageInfo colorImageInfo{};
			colorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			colorImageInfo.imageView = opaqueImageViews[i];

			VkDescriptorImageInfo accumImageInfo{};
			accumImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			accumImageInfo.imageView = m_SwapChainResourcesByCamera[camera][i].colorAccumView;

			VkDescriptorImageInfo revealImageInfo{};
			revealImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			revealImageInfo.imageView = m_SwapChainResourcesByCamera[camera][i].alphaAccumView;

			VkDescriptorBufferInfo frameBufferInfo{};
			frameBufferInfo.buffer = m_SwapChainResourcesByCamera[camera][i].frameInfoBuffer;
			frameBufferInfo.offset = 0;
			frameBufferInfo.range = sizeof(glm::vec2);

			std::array<VkWriteDescriptorSet, 4> descriptorWrites{};
			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = m_SwapChainResourcesByCamera[camera][i].compositeDescriptorSet;
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pImageInfo = &colorImageInfo;

			descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].dstSet = m_SwapChainResourcesByCamera[camera][i].compositeDescriptorSet;
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].dstArrayElement = 0;
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			descriptorWrites[1].descriptorCount = 1;
			descriptorWrites[1].pImageInfo = &accumImageInfo;

			descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[2].dstSet = m_SwapChainResourcesByCamera[camera][i].compositeDescriptorSet;
			descriptorWrites[2].dstBinding = 2;
			descriptorWrites[2].dstArrayElement = 0;
			descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			descriptorWrites[2].descriptorCount = 1;
			descriptorWrites[2].pImageInfo = &revealImageInfo;

			descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[3].dstSet = m_SwapChainResourcesByCamera[camera][i].compositeDescriptorSet;
			descriptorWrites[3].dstBinding = 3;
			descriptorWrites[3].dstArrayElement = 0;
			descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[3].descriptorCount = 1;
			descriptorWrites[3].pBufferInfo = &frameBufferInfo;

			vkUpdateDescriptorSets(context->getDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}
	}

	void TransparentPass::hotReloadShaders(const VulkanContext* context)
	{
		vkDestroyPipelineLayout(context->getDevice(), m_GraphicsPipelineLayout, nullptr);
		for (auto [permutation, pipeline] : m_PipelineByMaterialPermutation) {
			vkDestroyPipeline(context->getDevice(), pipeline, nullptr);
		}

		vkDestroyPipelineLayout(context->getDevice(), m_CompositePipelineLayout, nullptr);
		vkDestroyPipeline(context->getDevice(), m_CompositePipeline, nullptr);

		m_PipelineByMaterialPermutation.clear();

		createGraphicsPipelines(context);
	}

	void TransparentPass::createImages(const VulkanContext* context, std::shared_ptr<Camera> camera)
	{
		size_t swapChainImageCount = context->getAmountOfSwapChainImages();

		m_SwapChainResourcesByCamera[camera].resize(swapChainImageCount);

		for (int i = 0; i < swapChainImageCount; i++) {
			context->createImage(camera->viewportSize.x, camera->viewportSize.y, 1, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, m_SwapChainResourcesByCamera[camera][i].colorAccumTarget, m_SwapChainResourcesByCamera[camera][i].colorAccumAllocation);
			m_SwapChainResourcesByCamera[camera][i].colorAccumView = context->createImageView(m_SwapChainResourcesByCamera[camera][i].colorAccumTarget, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, 1);
		}

		for (int i = 0; i < swapChainImageCount; i++) {
			context->createImage(camera->viewportSize.x, camera->viewportSize.y, 1, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R16_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, m_SwapChainResourcesByCamera[camera][i].alphaAccumTarget, m_SwapChainResourcesByCamera[camera][i].alphaAccumAllocation);
			m_SwapChainResourcesByCamera[camera][i].alphaAccumView = context->createImageView(m_SwapChainResourcesByCamera[camera][i].alphaAccumTarget, VK_FORMAT_R16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, 1);
		}

		for (int i = 0; i < swapChainImageCount; i++) {
			context->createImage(camera->viewportSize.x, camera->viewportSize.y, 1, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, m_SwapChainResourcesByCamera[camera][i].compositeTarget, m_SwapChainResourcesByCamera[camera][i].compositeAllocation);
			m_SwapChainResourcesByCamera[camera][i].compositeView = context->createImageView(m_SwapChainResourcesByCamera[camera][i].compositeTarget, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, 1);
		}
	}

	void TransparentPass::createBuffers(const VulkanContext* context, std::shared_ptr<Camera> camera)
	{
		VkDeviceSize bufferSize = sizeof(glm::vec2);

		for (size_t i = 0; i < context->getAmountOfSwapChainImages(); i++) {
			// TODO: use the VMA_ALLOCATION_CREATE_MAPPED_BIT along with VmaAllocationInfo object instead
			context->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, m_SwapChainResourcesByCamera[camera][i].frameInfoBuffer, m_SwapChainResourcesByCamera[camera][i].frameInfoAllocation);
			vmaMapMemory(context->getAllocator(), m_SwapChainResourcesByCamera[camera][i].frameInfoAllocation, &m_SwapChainResourcesByCamera[camera][i].frameInfoMapped);
		}
	}

	void TransparentPass::createRenderPasses(const VulkanContext* context)
	{
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = VK_FORMAT_R16G16B16A16_SFLOAT;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkAttachmentDescription alphaAttachment{};
		alphaAttachment.format = VK_FORMAT_R16_SFLOAT;
		alphaAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		alphaAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		alphaAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		alphaAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		alphaAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkAttachmentDescription depthAttachment{};
		depthAttachment.format = context->findDepthFormat();
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference alphaAttachmentRef{};
		alphaAttachmentRef.attachment = 1;
		alphaAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef{};
		depthAttachmentRef.attachment = 2;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		//TODO: add subpass dependency for optimisation

		std::array<VkAttachmentReference, 2> colorRefs = { colorAttachmentRef, alphaAttachmentRef };
		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 2;
		subpass.pColorAttachments = colorRefs.data();
		subpass.pDepthStencilAttachment = &depthAttachmentRef;

		std::array<VkAttachmentDescription, 3> attachments = { colorAttachment, alphaAttachment, depthAttachment };
		VkRenderPassCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		createInfo.attachmentCount = 3;
		createInfo.pAttachments = attachments.data();
		createInfo.subpassCount = 1;
		createInfo.pSubpasses = &subpass;

		VkResult res = vkCreateRenderPass(context->getDevice(), &createInfo, nullptr, &m_RenderPass);
		REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to create render pass");

		VkAttachmentDescription compositeAttachment{};
		compositeAttachment.format = VK_FORMAT_R16G16B16A16_SFLOAT;
		compositeAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		compositeAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		compositeAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		compositeAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		compositeAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkAttachmentReference compositeAttachmentRef{};
		compositeAttachmentRef.attachment = 0;
		compositeAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDependency dependency{};
		dependency.srcSubpass = 0;
		dependency.dstSubpass = VK_SUBPASS_EXTERNAL;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependency.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dependency.dependencyFlags = 0;

		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &compositeAttachmentRef;
		subpass.pDepthStencilAttachment = nullptr;

		createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		createInfo.attachmentCount = 1;
		createInfo.pAttachments = &compositeAttachment;
		createInfo.subpassCount = 1;
		createInfo.pSubpasses = &subpass;
		createInfo.dependencyCount = 1;
		createInfo.pDependencies = &dependency;

		res = vkCreateRenderPass(context->getDevice(), &createInfo, nullptr, &m_CompositeRenderPass);
		REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to create render pass");
	}

	void TransparentPass::createFrameBuffers(const VulkanContext* context,
		std::shared_ptr<Camera> camera,
		std::vector<VkImageView>& endImageViews,
		std::vector<VkImageView>& depthImageViews)
	{
		for (size_t i = 0; i < context->getAmountOfSwapChainImages(); i++) {
			std::array<VkImageView, 3> attachments = {
				m_SwapChainResourcesByCamera[camera][i].colorAccumView,
				m_SwapChainResourcesByCamera[camera][i].alphaAccumView,
				depthImageViews[i]
			};

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = m_RenderPass;
			framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			framebufferInfo.pAttachments = attachments.data();
			framebufferInfo.width = camera->viewportSize.x;
			framebufferInfo.height = camera->viewportSize.y;
			framebufferInfo.layers = 1;

			VkResult res = vkCreateFramebuffer(context->getDevice(), &framebufferInfo, nullptr, &m_SwapChainResourcesByCamera[camera][i].framebuffer);
			REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to create framebuffer");
		}

		for (size_t i = 0; i < context->getAmountOfSwapChainImages(); i++) {
			std::array<VkImageView, 1> attachments = {
				endImageViews[i],
			};

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = m_CompositeRenderPass;
			framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			framebufferInfo.pAttachments = attachments.data();
			framebufferInfo.width = camera->viewportSize.x;
			framebufferInfo.height = camera->viewportSize.y;
			framebufferInfo.layers = 1;

			VkResult res = vkCreateFramebuffer(context->getDevice(), &framebufferInfo, nullptr, &m_SwapChainResourcesByCamera[camera][i].compositeFramebuffer);
			REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to create framebuffer");
		}
	}


	void TransparentPass::createGraphicsPipelines(const VulkanContext* context)
	{
		uint32_t mostUsedFlags = AlbedoTexture | NormalTexture | MetallicRoughnessTexture;

		std::vector<uint32_t> prebakePermutations{
			0,
			NormalTexture | MetallicRoughnessTexture,
			MetallicRoughnessTexture,
			AlbedoTexture | NormalTexture,
		};

		auto vertShaderCode = Shader::CompileHLSLToSPIRV("Assets/Shaders/Transparent.vert", mostUsedFlags);
		auto fragShaderCode = Shader::CompileHLSLToSPIRV("Assets/Shaders/Transparent.frag", mostUsedFlags);

		if (vertShaderCode.empty() || fragShaderCode.empty()) {
			REON_CORE_ERROR("Shader code not compiled correctly");
			return;
		}

		VkShaderModule vertShaderModule = context->createShaderModule(vertShaderCode);
		VkShaderModule fragShaderModule = context->createShaderModule(fragShaderCode);

		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertShaderModule;
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

		auto bindingDescription = Vertex::getBindingDescription();
		auto attributeDescriptions = Vertex::getAttributeDescriptions();

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;

		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f;
		rasterizer.depthBiasClamp = 0.0f;
		rasterizer.depthBiasSlopeFactor = 0.0f;

		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampling.minSampleShading = 0.2f;
		multisampling.pSampleMask = nullptr;
		multisampling.alphaToCoverageEnable = VK_FALSE;
		multisampling.alphaToOneEnable = VK_FALSE;

		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_TRUE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

		VkPipelineColorBlendAttachmentState alphaBlendAttachment{};
		alphaBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT;
		alphaBlendAttachment.blendEnable = VK_TRUE;
		alphaBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		alphaBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
		alphaBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		alphaBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		alphaBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
		alphaBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

		std::array< VkPipelineColorBlendAttachmentState, 2> attachments = { colorBlendAttachment, alphaBlendAttachment };
		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = 2;
		colorBlending.pAttachments = attachments.data();
		colorBlending.blendConstants[0] = 0.0f;
		colorBlending.blendConstants[1] = 0.0f;
		colorBlending.blendConstants[2] = 0.0f;
		colorBlending.blendConstants[3] = 0.0f;

		VkPipelineDepthStencilStateCreateInfo depthStencil{};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = VK_FALSE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.stencilTestEnable = VK_FALSE;

		std::vector<VkDynamicState> dynamicStates = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR,
			VK_DYNAMIC_STATE_CULL_MODE
		};

		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = m_Layouts.size();
		pipelineLayoutInfo.pSetLayouts = m_Layouts.data();
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.pPushConstantRanges = nullptr;

		VkResult res = vkCreatePipelineLayout(context->getDevice(), &pipelineLayoutInfo, nullptr, &m_GraphicsPipelineLayout);
		REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to create pipeline layout")

			VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = &depthStencil;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = &dynamicState;
		pipelineInfo.layout = m_GraphicsPipelineLayout;
		pipelineInfo.renderPass = m_RenderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex = -1;
		pipelineInfo.flags = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;

		res = vkCreateGraphicsPipelines(context->getDevice(), m_PipelineCache, 1, &pipelineInfo, nullptr, &m_GraphicsPipeline);
		REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to create graphics pipeline")

			vkDestroyShaderModule(context->getDevice(), fragShaderModule, nullptr);
		vkDestroyShaderModule(context->getDevice(), vertShaderModule, nullptr);

		m_PipelineByMaterialPermutation.insert({ mostUsedFlags, m_GraphicsPipeline });

		for (auto permutation : prebakePermutations) {
			m_PipelineByMaterialPermutation.insert({ permutation, createPermutationGraphicsPipeline(context, m_GraphicsPipeline, permutation) });
		}

		vertShaderCode = Shader::CompileHLSLToSPIRV("Assets/Shaders/fullScreen.vert", 0);
		fragShaderCode = Shader::CompileHLSLToSPIRV("Assets/Shaders/WBOITComposite.frag", 0);

		if (vertShaderCode.empty() || fragShaderCode.empty()) {
			REON_CORE_ERROR("Shader code not compiled correctly");
			return;
		}

		vertShaderModule = context->createShaderModule(vertShaderCode);
		fragShaderModule = context->createShaderModule(fragShaderCode);

		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertShaderModule;
		vertShaderStageInfo.pName = "main";

		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo compositeShaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 0;
		vertexInputInfo.pVertexBindingDescriptions = nullptr;
		vertexInputInfo.vertexAttributeDescriptionCount = 0;
		vertexInputInfo.pVertexAttributeDescriptions = nullptr;

		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;

		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f;
		rasterizer.depthBiasClamp = 0.0f;
		rasterizer.depthBiasSlopeFactor = 0.0f;

		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampling.minSampleShading = 0.2f;
		multisampling.pSampleMask = nullptr;
		multisampling.alphaToCoverageEnable = VK_FALSE;
		multisampling.alphaToOneEnable = VK_FALSE;

		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f;
		colorBlending.blendConstants[1] = 0.0f;
		colorBlending.blendConstants[2] = 0.0f;
		colorBlending.blendConstants[3] = 0.0f;

		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_FALSE;
		depthStencil.depthWriteEnable = VK_FALSE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_ALWAYS;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.stencilTestEnable = VK_FALSE;

		dynamicStates = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR,
		};

		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = 2;
		dynamicState.pDynamicStates = dynamicStates.data();

		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &m_CompositeDescriptorSetLayout;
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.pPushConstantRanges = nullptr;

		res = vkCreatePipelineLayout(context->getDevice(), &pipelineLayoutInfo, nullptr, &m_CompositePipelineLayout);
		REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to create pipeline layout")

			pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = compositeShaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = &depthStencil;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = &dynamicState;
		pipelineInfo.layout = m_CompositePipelineLayout;
		pipelineInfo.renderPass = m_CompositeRenderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex = -1;
		pipelineInfo.flags = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;

		res = vkCreateGraphicsPipelines(context->getDevice(), m_PipelineCache, 1, &pipelineInfo, nullptr, &m_CompositePipeline);
		REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to create graphics pipeline")

			vkDestroyShaderModule(context->getDevice(), fragShaderModule, nullptr);
		vkDestroyShaderModule(context->getDevice(), vertShaderModule, nullptr);
	}

	void TransparentPass::createDescriptorSetLayouts(const VulkanContext* context)
	{
		VkDescriptorSetLayoutBinding opaqueTexLayoutBinding{};
		opaqueTexLayoutBinding.binding = 0;
		opaqueTexLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		opaqueTexLayoutBinding.descriptorCount = 1;
		opaqueTexLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		opaqueTexLayoutBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding accumTexLayoutBinding{};
		accumTexLayoutBinding.binding = 1;
		accumTexLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		accumTexLayoutBinding.descriptorCount = 1;
		accumTexLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		accumTexLayoutBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding revealTexLayoutBinding{};
		revealTexLayoutBinding.binding = 2;
		revealTexLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		revealTexLayoutBinding.descriptorCount = 1;
		revealTexLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		revealTexLayoutBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding frameInfoLayoutBinding{};
		frameInfoLayoutBinding.binding = 3;
		frameInfoLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		frameInfoLayoutBinding.descriptorCount = 1;
		frameInfoLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		frameInfoLayoutBinding.pImmutableSamplers = nullptr;

		std::array< VkDescriptorSetLayoutBinding, 4> bindings = { opaqueTexLayoutBinding, accumTexLayoutBinding , revealTexLayoutBinding, frameInfoLayoutBinding };
		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = bindings.size();
		layoutInfo.pBindings = bindings.data();

		VkResult res = vkCreateDescriptorSetLayout(context->getDevice(), &layoutInfo, nullptr, &m_CompositeDescriptorSetLayout);
		REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to create descriptor set layout");
	}

	void TransparentPass::createDescriptorSets(const VulkanContext* context, std::shared_ptr<Camera> camera, std::vector<VkImageView> opaqueImageViews)
	{
		std::vector<VkDescriptorSetLayout> compositeLayouts(context->getAmountOfSwapChainImages(), m_CompositeDescriptorSetLayout);
		VkDescriptorSetAllocateInfo compositeAllocInfo{};
		compositeAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		compositeAllocInfo.descriptorPool = context->getDescriptorPool();
		compositeAllocInfo.descriptorSetCount = static_cast<uint32_t>(context->getAmountOfSwapChainImages());
		compositeAllocInfo.pSetLayouts = compositeLayouts.data();

		std::vector<VkDescriptorSet> allSets(compositeLayouts.size());
		VkResult res = vkAllocateDescriptorSets(context->getDevice(), &compositeAllocInfo, allSets.data());
		REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to allocate descriptor sets");

		for (size_t i = 0; i < context->getAmountOfSwapChainImages(); i++) {
			m_SwapChainResourcesByCamera[camera][i].compositeDescriptorSet = allSets[i];

			VkDescriptorImageInfo colorImageInfo{};
			colorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			colorImageInfo.imageView = opaqueImageViews[i];

			VkDescriptorImageInfo accumImageInfo{};
			accumImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			accumImageInfo.imageView = m_SwapChainResourcesByCamera[camera][i].colorAccumView;

			VkDescriptorImageInfo revealImageInfo{};
			revealImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			revealImageInfo.imageView = m_SwapChainResourcesByCamera[camera][i].alphaAccumView;

			VkDescriptorBufferInfo frameBufferInfo{};
			frameBufferInfo.buffer = m_SwapChainResourcesByCamera[camera][i].frameInfoBuffer;
			frameBufferInfo.offset = 0;
			frameBufferInfo.range = sizeof(glm::vec2);

			std::array<VkWriteDescriptorSet, 4> descriptorWrites{};
			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = m_SwapChainResourcesByCamera[camera][i].compositeDescriptorSet;
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pImageInfo = &colorImageInfo;

			descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].dstSet = m_SwapChainResourcesByCamera[camera][i].compositeDescriptorSet;
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].dstArrayElement = 0;
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			descriptorWrites[1].descriptorCount = 1;
			descriptorWrites[1].pImageInfo = &accumImageInfo;

			descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[2].dstSet = m_SwapChainResourcesByCamera[camera][i].compositeDescriptorSet;
			descriptorWrites[2].dstBinding = 2;
			descriptorWrites[2].dstArrayElement = 0;
			descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			descriptorWrites[2].descriptorCount = 1;
			descriptorWrites[2].pImageInfo = &revealImageInfo;

			descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[3].dstSet = m_SwapChainResourcesByCamera[camera][i].compositeDescriptorSet;
			descriptorWrites[3].dstBinding = 3;
			descriptorWrites[3].dstArrayElement = 0;
			descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[3].descriptorCount = 1;
			descriptorWrites[3].pBufferInfo = &frameBufferInfo;

			vkUpdateDescriptorSets(context->getDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}
	}

	VkPipeline TransparentPass::getPipelineFromFlags(const VulkanContext* context, uint32_t flags)
	{
		if (m_PipelineByMaterialPermutation.contains(flags)) {
			return m_PipelineByMaterialPermutation[flags];
		}
		else {
			VkPipeline newPipeline = createPermutationGraphicsPipeline(context, m_GraphicsPipeline, flags);
			m_PipelineByMaterialPermutation[flags] = newPipeline;
			return newPipeline;
		}
	}

	void TransparentPass::cleanForResize(const VulkanContext* context, std::shared_ptr<Camera> camera)
	{
		for (int i = 0; i < context->getAmountOfSwapChainImages(); i++) {
			vkDestroyImage(context->getDevice(), m_SwapChainResourcesByCamera[camera][i].colorAccumTarget, nullptr);
			vmaFreeMemory(context->getAllocator(), m_SwapChainResourcesByCamera[camera][i].colorAccumAllocation);
			vkDestroyImageView(context->getDevice(), m_SwapChainResourcesByCamera[camera][i].colorAccumView, nullptr);

			vkDestroyImage(context->getDevice(), m_SwapChainResourcesByCamera[camera][i].alphaAccumTarget, nullptr);
			vmaFreeMemory(context->getAllocator(), m_SwapChainResourcesByCamera[camera][i].alphaAccumAllocation);
			vkDestroyImageView(context->getDevice(), m_SwapChainResourcesByCamera[camera][i].alphaAccumView, nullptr);

			vkDestroyImage(context->getDevice(), m_SwapChainResourcesByCamera[camera][i].compositeTarget, nullptr);
			vmaFreeMemory(context->getAllocator(), m_SwapChainResourcesByCamera[camera][i].compositeAllocation);
			vkDestroyImageView(context->getDevice(), m_SwapChainResourcesByCamera[camera][i].compositeView, nullptr);

			vkDestroyFramebuffer(context->getDevice(), m_SwapChainResourcesByCamera[camera][i].framebuffer, nullptr);
			vkDestroyFramebuffer(context->getDevice(), m_SwapChainResourcesByCamera[camera][i].compositeFramebuffer, nullptr);
		}
	}

	void TransparentPass::createSyncObjects(const VulkanContext* context)
	{
		m_TransparentRenderDone.resize(context->MAX_FRAMES_IN_FLIGHT);

		VkSemaphoreCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		for (int i = 0; i < context->MAX_FRAMES_IN_FLIGHT; i++) {
			vkCreateSemaphore(context->getDevice(), &createInfo, nullptr, &m_TransparentRenderDone[i]);
		}
	}

	VkPipeline TransparentPass::createPermutationGraphicsPipeline(const VulkanContext* context, VkPipeline basePipeline, uint32_t flags)
	{
		auto vertShaderCode = Shader::CompileHLSLToSPIRV("Assets/Shaders/Transparent.vert", flags);
		auto fragShaderCode = Shader::CompileHLSLToSPIRV("Assets/Shaders/Transparent.frag", flags);

		if (vertShaderCode.empty() || fragShaderCode.empty()) {
			REON_CORE_ERROR("Shader code not compiled correctly");
			return {};
		}

		VkShaderModule vertShaderModule = context->createShaderModule(vertShaderCode);
		VkShaderModule fragShaderModule = context->createShaderModule(fragShaderCode);

		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertShaderModule;
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

		auto bindingDescription = Vertex::getBindingDescription();
		auto attributeDescriptions = Vertex::getAttributeDescriptions();

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;

		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f;
		rasterizer.depthBiasClamp = 0.0f;
		rasterizer.depthBiasSlopeFactor = 0.0f;

		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampling.minSampleShading = 0.2f;
		multisampling.pSampleMask = nullptr;
		multisampling.alphaToCoverageEnable = VK_FALSE;
		multisampling.alphaToOneEnable = VK_FALSE;

		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_TRUE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

		VkPipelineColorBlendAttachmentState alphaBlendAttachment{};
		alphaBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT;
		alphaBlendAttachment.blendEnable = VK_TRUE;
		alphaBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		alphaBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
		alphaBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		alphaBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		alphaBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
		alphaBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

		std::array< VkPipelineColorBlendAttachmentState, 2> attachments = { colorBlendAttachment, alphaBlendAttachment };
		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = 2;
		colorBlending.pAttachments = attachments.data();
		colorBlending.blendConstants[0] = 0.0f;
		colorBlending.blendConstants[1] = 0.0f;
		colorBlending.blendConstants[2] = 0.0f;
		colorBlending.blendConstants[3] = 0.0f;

		VkPipelineDepthStencilStateCreateInfo depthStencil{};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = VK_FALSE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.stencilTestEnable = VK_FALSE;

		std::vector<VkDynamicState> dynamicStates = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR,
			VK_DYNAMIC_STATE_CULL_MODE
		};

		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = m_Layouts.size();
		pipelineLayoutInfo.pSetLayouts = m_Layouts.data();
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.pPushConstantRanges = nullptr;

		VkResult res = vkCreatePipelineLayout(context->getDevice(), &pipelineLayoutInfo, nullptr, &m_GraphicsPipelineLayout);
		REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to create pipeline layout")

			VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = &depthStencil;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = &dynamicState;
		pipelineInfo.layout = m_GraphicsPipelineLayout;
		pipelineInfo.renderPass = m_RenderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = basePipeline;
		pipelineInfo.basePipelineIndex = -1;
		pipelineInfo.flags = VK_PIPELINE_CREATE_DERIVATIVE_BIT;

		VkPipeline pipeline;

		res = vkCreateGraphicsPipelines(context->getDevice(), m_PipelineCache, 1, &pipelineInfo, nullptr, &pipeline);
		REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to create graphics pipeline")

			vkDestroyShaderModule(context->getDevice(), fragShaderModule, nullptr);
		vkDestroyShaderModule(context->getDevice(), vertShaderModule, nullptr);

		return pipeline;
	}

}