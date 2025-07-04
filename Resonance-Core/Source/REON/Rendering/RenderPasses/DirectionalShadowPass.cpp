#include "reonpch.h"
#include "DirectionalShadowPass.h"
#include "REON/Rendering/Shader.h"
#include "REON/Rendering/Structs/Vertex.h"

namespace REON {
	void DirectionalShadowPass::Init(const VulkanContext* context)
	{
		context->createCommandPool(m_CommandPool, context->findQueueFamilies(context->getPhysicalDevice()).graphicsFamily.value());
		createCommandBuffers(context);
		createImages(context);
		createRenderPass(context);
		createFrameBuffers(context);
		createDescriptorSetLayout(context);
		createGraphicsPipeline(context);
		createPerLightBuffers(context);
	}

	void DirectionalShadowPass::render(const VulkanContext* context, std::vector<std::shared_ptr<Renderer>> renderers, glm::mat4 mainLightViewProj, VkSemaphore signalSemaphore)
	{
		int currentFrame = context->getCurrentFrame();
		int currentImageIndex = context->getCurrentImageIndex();

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0;
		beginInfo.pInheritanceInfo = nullptr;

		VkResult res = vkBeginCommandBuffer(m_CommandBuffers[currentFrame], &beginInfo);
		REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to begin recording command buffer");

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_RenderPass;
		renderPassInfo.framebuffer = m_Framebuffers[currentImageIndex];
		renderPassInfo.renderArea.offset = { 0,0 };
		renderPassInfo.renderArea.extent = { MAIN_SHADOW_WIDTH, MAIN_SHADOW_HEIGHT };

		std::array<VkClearValue, 1> clearValues{};
		clearValues[0].depthStencil = { 1.0f, 0 };
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = m_DepthImages[currentImageIndex];
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		vkCmdPipelineBarrier(m_CommandBuffers[currentFrame],
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		vkCmdBeginRenderPass(m_CommandBuffers[currentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(m_CommandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(MAIN_SHADOW_WIDTH);
		viewport.height = static_cast<float>(MAIN_SHADOW_HEIGHT);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(m_CommandBuffers[currentFrame], 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = { 0,0 };
		scissor.extent = { MAIN_SHADOW_WIDTH, MAIN_SHADOW_HEIGHT };
		vkCmdSetScissor(m_CommandBuffers[currentFrame], 0, 1, &scissor);

		memcpy(m_PerLightBuffersMapped[currentFrame], &mainLightViewProj, sizeof(mainLightViewProj));

		for (const auto& renderer : renderers) {

			for (const auto& cmd : renderer->drawCommands) {
				if (!(cmd.material->blendingMode == Mask || cmd.material->renderingMode == Opaque))
					continue;

				std::array<VkDescriptorSet, 2> descriptorSets{ m_PerLightDescriptorSets[currentFrame], cmd.owner->shadowObjectDescriptorSets[context->getCurrentFrame()] };
				auto modelMatrix = cmd.owner->getModelMatrix();
				memcpy(cmd.owner->shadowObjectDataBuffersMapped[context->getCurrentFrame()], &modelMatrix, sizeof(glm::mat4));

				VkBuffer vertexBuffers[] = { cmd.mesh->m_VertexBuffer };
				VkDeviceSize offsets[] = { 0 };
				vkCmdBindVertexBuffers(m_CommandBuffers[currentFrame], 0, 1, vertexBuffers, offsets);

				vkCmdBindIndexBuffer(m_CommandBuffers[currentFrame], cmd.mesh->m_IndexBuffer, 0, VK_INDEX_TYPE_UINT32);

				vkCmdBindDescriptorSets(m_CommandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, descriptorSets.size(), descriptorSets.data(), 0, nullptr);
				vkCmdDrawIndexed(m_CommandBuffers[currentFrame], static_cast<uint32_t>(cmd.indexCount), 1, cmd.startIndex, 0, 0);
			}



			//set object buffers (model matrix)
		}

		vkCmdEndRenderPass(m_CommandBuffers[currentFrame]);

		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = m_DepthImages[currentImageIndex];
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(m_CommandBuffers[currentFrame],
			VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		res = vkEndCommandBuffer(m_CommandBuffers[currentFrame]);
		REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to record command buffer");

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_CommandBuffers[currentFrame];

		VkSemaphore signalSemaphores[] = { signalSemaphore };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		res = vkQueueSubmit(context->getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
		REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to submit draw command buffer");

	}

	void DirectionalShadowPass::cleanup(const VulkanContext* context)
	{
		for (int i = 0; i < context->getAmountOfSwapChainImages(); i++) {
			vkDestroyImage(context->getDevice(), m_DepthImages[i], nullptr);
			vmaFreeMemory(context->getAllocator(), m_DepthImageAllocations[i]);
			vkDestroyImageView(context->getDevice(), m_DepthImageViews[i], nullptr);
			vkDestroyFramebuffer(context->getDevice(), m_Framebuffers[i], nullptr);
		}

		vkDestroySampler(context->getDevice(), m_DepthImageSampler, nullptr);

		for (int i = 0; i < context->MAX_FRAMES_IN_FLIGHT; i++) {
			vkDestroyBuffer(context->getDevice(), m_PerLightBuffers[i], nullptr);
			vmaUnmapMemory(context->getAllocator(), m_PerLightBufferAllocations[i]);
			vmaFreeMemory(context->getAllocator(), m_PerLightBufferAllocations[i]);
		}

		vkDestroyDescriptorSetLayout(context->getDevice(), m_PerLightDescriptorSetLayout, nullptr);
		vkDestroyDescriptorSetLayout(context->getDevice(), m_PerObjectDescriptorSetLayout, nullptr);

		vkDestroyPipeline(context->getDevice(), m_GraphicsPipeline, nullptr);
		vkDestroyPipelineLayout(context->getDevice(), m_PipelineLayout, nullptr);

		vkDestroyRenderPass(context->getDevice(), m_RenderPass, nullptr);
		
		vkDestroyCommandPool(context->getDevice(), m_CommandPool, nullptr);
	}

	void DirectionalShadowPass::createCommandBuffers(const VulkanContext* context)
	{
		m_CommandBuffers.resize(context->MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = m_CommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = static_cast<uint32_t>(m_CommandBuffers.size());

		VkResult res = vkAllocateCommandBuffers(context->getDevice(), &allocInfo, m_CommandBuffers.data());
		REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to allocate command buffers");
	}

	void DirectionalShadowPass::createImages(const VulkanContext* context)
	{
		size_t swapChainImageCount = context->getAmountOfSwapChainImages();

		m_DepthImages.resize(swapChainImageCount);
		m_DepthImageViews.resize(swapChainImageCount);
		m_DepthImageAllocations.resize(swapChainImageCount);

		for (int i = 0; i < swapChainImageCount; i++) {
			context->createImage(MAIN_SHADOW_WIDTH, MAIN_SHADOW_HEIGHT, 1, VK_SAMPLE_COUNT_1_BIT, context->findDepthFormat(), VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, m_DepthImages[i], m_DepthImageAllocations[i]);
			m_DepthImageViews[i] = context->createImageView(m_DepthImages[i], context->findDepthFormat(), VK_IMAGE_ASPECT_DEPTH_BIT, 1);
		}

		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;


		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(context->getPhysicalDevice(), &properties); // TODO: IF I USE THIS MORE STORE IT AT THE START AT CREATION

		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
		samplerInfo.mipLodBias = 0.0f;

		VkResult res = vkCreateSampler(context->getDevice(), &samplerInfo, nullptr, &m_DepthImageSampler);
		REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to create texture sampler");
	}

	void DirectionalShadowPass::createRenderPass(const VulkanContext* context)
	{
		VkAttachmentDescription depthAttachment{};
		depthAttachment.format = context->findDepthFormat();
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef{};
		depthAttachmentRef.attachment = 0;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependency.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 0;
		subpass.pDepthStencilAttachment = &depthAttachmentRef;

		std::array<VkAttachmentDescription, 1> attachments = { depthAttachment };
		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		VkResult res = vkCreateRenderPass(context->getDevice(), &renderPassInfo, nullptr, &m_RenderPass);
		REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to create render pass")
	}

	void DirectionalShadowPass::createFrameBuffers(const VulkanContext* context)
	{
		m_Framebuffers.resize(context->getSwapChainImageViews().size());

		for (size_t i = 0; i < context->getSwapChainImageViews().size(); i++) {
			std::array<VkImageView, 1> attachments = {
				m_DepthImageViews[i],
			};

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = m_RenderPass;
			framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			framebufferInfo.pAttachments = attachments.data();
			framebufferInfo.width = MAIN_SHADOW_WIDTH;
			framebufferInfo.height = MAIN_SHADOW_HEIGHT;
			framebufferInfo.layers = 1;

			VkResult res = vkCreateFramebuffer(context->getDevice(), &framebufferInfo, nullptr, &m_Framebuffers[i]);
			REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to create framebuffer");
		}
	}

	void DirectionalShadowPass::createDescriptorSetLayout(const VulkanContext* context)
	{
		VkDescriptorSetLayoutBinding perLightLayoutBinding{};
		perLightLayoutBinding.binding = 0;
		perLightLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		perLightLayoutBinding.descriptorCount = 1;
		perLightLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		perLightLayoutBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = 1;
		layoutInfo.pBindings = &perLightLayoutBinding;

		VkResult res = vkCreateDescriptorSetLayout(context->getDevice(), &layoutInfo, nullptr, &m_PerLightDescriptorSetLayout);
		REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to create descriptor set layout");

		VkDescriptorSetLayoutBinding perObjectLayoutBinding{};
		perObjectLayoutBinding.binding = 1;
		perObjectLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		perObjectLayoutBinding.descriptorCount = 1;
		perObjectLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		perObjectLayoutBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutCreateInfo shadowLayoutInfo{};
		shadowLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		shadowLayoutInfo.bindingCount = 1;
		shadowLayoutInfo.pBindings = &perObjectLayoutBinding;

		res = vkCreateDescriptorSetLayout(context->getDevice(), &shadowLayoutInfo, nullptr, &m_PerObjectDescriptorSetLayout);
		REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to create descriptor set layout");
	}

	void DirectionalShadowPass::createGraphicsPipeline(const VulkanContext* context)
	{
		auto vertShaderCode = Shader::CompileHLSLToSPIRV("Assets/Shaders/DirectionalShadow.vert");
		auto fragShaderCode = Shader::CompileHLSLToSPIRV("Assets/Shaders/DirectionalShadow.frag");

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
		rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;
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
		colorBlendAttachment.blendEnable = VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f;
		colorBlending.blendConstants[1] = 0.0f;
		colorBlending.blendConstants[2] = 0.0f;
		colorBlending.blendConstants[3] = 0.0f;

		VkPipelineDepthStencilStateCreateInfo depthStencil{};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = VK_TRUE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.stencilTestEnable = VK_FALSE;
		
		std::vector<VkDynamicState> dynamicStates = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();

		std::array<VkDescriptorSetLayout, 2> setLayouts{ m_PerLightDescriptorSetLayout, m_PerObjectDescriptorSetLayout };
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = setLayouts.size();
		pipelineLayoutInfo.pSetLayouts = setLayouts.data();
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.pPushConstantRanges = nullptr;

		VkResult res = vkCreatePipelineLayout(context->getDevice(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout);
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
		pipelineInfo.layout = m_PipelineLayout;
		pipelineInfo.renderPass = m_RenderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex = -1;

		res = vkCreateGraphicsPipelines(context->getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_GraphicsPipeline);
		REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to create graphics pipeline");

		vkDestroyShaderModule(context->getDevice(), fragShaderModule, nullptr);
		vkDestroyShaderModule(context->getDevice(), vertShaderModule, nullptr);
	}

	void DirectionalShadowPass::createPerLightBuffers(const VulkanContext* context)
	{
		VkDeviceSize bufferSize = sizeof(glm::mat4);

		m_PerLightBuffers.resize(context->MAX_FRAMES_IN_FLIGHT);
		m_PerLightBufferAllocations.resize(context->MAX_FRAMES_IN_FLIGHT);
		m_PerLightBuffersMapped.resize(context->MAX_FRAMES_IN_FLIGHT);

		for (size_t i = 0; i < context->MAX_FRAMES_IN_FLIGHT; i++) {
			// TODO: use the VMA_ALLOCATION_CREATE_MAPPED_BIT along with VmaAllocationInfo object instead
			context->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, m_PerLightBuffers[i], m_PerLightBufferAllocations[i]);

			vmaMapMemory(context->getAllocator(), m_PerLightBufferAllocations[i], &m_PerLightBuffersMapped[i]);
		}
	}

	void DirectionalShadowPass::createPerLightDescriptorSets(const VulkanContext* context)
	{
		m_PerLightDescriptorSets.resize(context->MAX_FRAMES_IN_FLIGHT);



		std::vector<VkDescriptorSetLayout> globalLayouts(context->MAX_FRAMES_IN_FLIGHT, m_PerLightDescriptorSetLayout);
		VkDescriptorSetAllocateInfo globalAllocInfo{};
		globalAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		globalAllocInfo.descriptorPool = context->getDescriptorPool();
		globalAllocInfo.descriptorSetCount = static_cast<uint32_t>(context->MAX_FRAMES_IN_FLIGHT);
		globalAllocInfo.pSetLayouts = globalLayouts.data();

		VkResult res = vkAllocateDescriptorSets(context->getDevice(), &globalAllocInfo, m_PerLightDescriptorSets.data());
		REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to allocate descriptor sets");

		for (size_t i = 0; i < context->MAX_FRAMES_IN_FLIGHT; i++) {
			VkDescriptorBufferInfo globalBufferInfo{};
			globalBufferInfo.buffer = m_PerLightBuffers[i];
			globalBufferInfo.offset = 0;
			globalBufferInfo.range = sizeof(glm::mat4);

			std::array<VkWriteDescriptorSet, 1> descriptorWrites{};
			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = m_PerLightDescriptorSets[i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &globalBufferInfo;

			vkUpdateDescriptorSets(context->getDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}
	}

	void DirectionalShadowPass::createPerObjectDescriptorSets(const VulkanContext* context, std::shared_ptr<Renderer> renderer)
	{
		renderer->shadowObjectDescriptorSets.resize(context->MAX_FRAMES_IN_FLIGHT);

		std::vector<VkDescriptorSetLayout> objectLayouts(context->MAX_FRAMES_IN_FLIGHT, m_PerObjectDescriptorSetLayout);
		VkDescriptorSetAllocateInfo objectAllocInfo{};
		objectAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		objectAllocInfo.descriptorPool = context->getDescriptorPool();
		objectAllocInfo.descriptorSetCount = static_cast<uint32_t>(context->MAX_FRAMES_IN_FLIGHT);
		objectAllocInfo.pSetLayouts = objectLayouts.data();

		VkResult res = vkAllocateDescriptorSets(context->getDevice(), &objectAllocInfo, renderer->shadowObjectDescriptorSets.data());
		REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to allocate descriptor sets");

		for (size_t i = 0; i < context->MAX_FRAMES_IN_FLIGHT; i++) {
			VkDescriptorBufferInfo objectBufferInfo{};
			objectBufferInfo.buffer = renderer->shadowObjectDataBuffers[i];
			objectBufferInfo.offset = 0;
			objectBufferInfo.range = sizeof(glm::mat4);

			std::array<VkWriteDescriptorSet, 1> descriptorWrites{};
			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = renderer->shadowObjectDescriptorSets[i];
			descriptorWrites[0].dstBinding = 1;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &objectBufferInfo;

			vkUpdateDescriptorSets(context->getDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}
	}

}