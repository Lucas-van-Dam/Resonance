#include "reonpch.h"
#include "UnlitPass.h"
#include "REON/rendering/shader.h"
#include "REON/rendering/Structs/Vertex.h"
#include <REON/Rendering/RenderManager.h>


namespace REON {
	void UnlitPass::init(const VulkanContext* context, uint width, uint height, std::vector<VkImageView> endImageViews,
		VkPipelineCache pipelineCache, std::vector<VkDescriptorSetLayout> layouts) {
		m_Width = width;
		m_Height = height;
		m_PipelineCache = pipelineCache;
		m_DescriptorSetLayouts = layouts;

		context->createCommandPool(m_CommandPool, context->findQueueFamilies(context->getPhysicalDevice()).graphicsFamily.value());
		context->createCommandBuffers(m_CommandBuffers, m_CommandPool, context->MAX_FRAMES_IN_FLIGHT);
		createImages(context);
		createRenderPass(context);
		createFrameBuffers(context, endImageViews);

		vkCmdSetPolygonModeEXT =
			(PFN_vkCmdSetPolygonModeEXT)vkGetDeviceProcAddr(context->getDevice(), "vkCmdSetPolygonModeEXT");

		if (!vkCmdSetPolygonModeEXT) {
			REON_CORE_ERROR("Couldnt load set polygon mode ext func");
		}

		std::vector<uint32_t> flags{
			0,
			AlbedoTexture,
			EmissiveTexture,
			AlbedoTexture | EmissiveTexture
		};

		for (uint32_t flag : flags) {
			m_PipelineByMaterialPermutation.insert({ flag, createGraphicsPipeline(context, pipelineCache, flag) });
		}
	}

	void UnlitPass::render(const VulkanContext* context, 
		std::unordered_map<std::string, std::unordered_map<std::string, std::vector<DrawCommand>>> rendererMap,
		VkDescriptorSet globalDescriptorSet, VkSemaphore signalSemaphore, RenderMode renderMode)
	{
		int currentFrame = context->getCurrentFrame();
		int currentImageIndex = context->getCurrentImageIndex();

		for (const auto& pair : rendererMap) {
			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = 0;
			beginInfo.pInheritanceInfo = nullptr;

			VkResult res = vkBeginCommandBuffer(m_CommandBuffers[currentFrame], &beginInfo);
			REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to begin recording command buffer");

			VkRenderPassBeginInfo renderPassInfo{};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = m_RenderPass;
			renderPassInfo.framebuffer = m_Framebuffers[context->getCurrentImageIndex()];
			renderPassInfo.renderArea.offset = { 0,0 };
			renderPassInfo.renderArea.extent = { m_Width, m_Height };

			std::array<VkClearValue, 4> clearValues{};
			clearValues[0].color = { 0.1f, 0.1f, 0.1f, 1.0f };
			clearValues[1].depthStencil = { 1.0f, 0 };
			clearValues[2].color = { 0.1f, 0.1f, 0.1f, 1.0f };
			clearValues[3].depthStencil = { 1.0f, 0 };
			renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
			renderPassInfo.pClearValues = clearValues.data();

			vkCmdBeginRenderPass(m_CommandBuffers[currentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			const auto& materialFromShader = pair.second;
			for (const auto& material : materialFromShader) {
				//set material wide buffers/textures
                auto mat = material.second.front().material.Lock();
				if (!(mat->blendingMode == Mask || mat->renderingMode == Opaque)) {
					continue;
				}
				uint32_t mask = AlbedoTexture | EmissiveTexture;
				auto pipeline = getPipelineFromFlags(context, mat->materialFlags & mask);
				if (pipeline == VK_NULL_HANDLE) {
					REON_CORE_WARN("Cant render because pipeline is not found");
					continue;
				}
				vkCmdBindPipeline(m_CommandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

				VkViewport viewport{};
				viewport.x = 0.0f;
				viewport.y = 0.0f;
				viewport.width = static_cast<float>(m_Width);
				viewport.height = static_cast<float>(m_Height);
				viewport.minDepth = 0.0f;
				viewport.maxDepth = 1.0f;
				vkCmdSetViewport(m_CommandBuffers[currentFrame], 0, 1, &viewport);

				VkRect2D scissor{};
				scissor.offset = { 0,0 };
				scissor.extent = { m_Width, m_Height };
				vkCmdSetScissor(m_CommandBuffers[currentFrame], 0, 1, &scissor);

				vkCmdSetPolygonModeEXT(m_CommandBuffers[currentFrame], renderMode == WIREFRAME ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL);

				FlatData data = mat->flatData;

				if (renderMode == WIREFRAME) 
				{ 
					data.albedo = glm::vec4(0.0, 0.0, 0.0, 1.0);
					data.emissiveFactor = glm::vec4(0.0, 0.0, 0.0, 1.0);
				}

				memcpy(mat->flatDataBuffersMapped[currentFrame], &data, sizeof(mat->flatData));

				vkCmdSetCullMode(m_CommandBuffers[currentFrame], mat->getDoubleSided() ? VK_CULL_MODE_NONE : VK_CULL_MODE_BACK_BIT);


				for (const auto& cmd : material.second) {
                    auto mesh = cmd.mesh.Lock();
                    if (!mesh)
                        continue;

					std::vector<VkDescriptorSet> descriptorSets = { globalDescriptorSet, mat->descriptorSets[currentFrame], cmd.owner->objectDescriptorSets[currentFrame] };

					ObjectRenderData data{};
					data.model = cmd.owner->getModelMatrix();
					data.transposeInverseModel = cmd.owner->getTransposeInverseModelMatrix();
					memcpy(cmd.owner->objectDataBuffersMapped[context->getCurrentFrame()], &data, sizeof(data));

					VkBuffer vertexBuffers[] = { mesh->m_VertexBuffer };
					VkDeviceSize offsets[] = { 0 };
					vkCmdBindVertexBuffers(m_CommandBuffers[currentFrame], 0, 1, vertexBuffers, offsets);

					vkCmdBindIndexBuffer(m_CommandBuffers[currentFrame], mesh->m_IndexBuffer, 0, VK_INDEX_TYPE_UINT32);

					vkCmdBindDescriptorSets(m_CommandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, descriptorSets.size(), descriptorSets.data(), 0, nullptr);
					vkCmdDrawIndexed(m_CommandBuffers[currentFrame], static_cast<uint32_t>(cmd.indexCount), 1, cmd.startIndex, 0, 0);
				}
			}

			vkCmdEndRenderPass(m_CommandBuffers[currentFrame]);

			res = vkEndCommandBuffer(m_CommandBuffers[currentFrame]);
			REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to record command buffer");

			VkSubmitInfo submitInfo{};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

			VkSemaphore waitSemaphores[] = { context->getCurrentImageAvailableSemaphore() };
			VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
			submitInfo.waitSemaphoreCount = 1;
			submitInfo.pWaitSemaphores = waitSemaphores;
			submitInfo.pWaitDstStageMask = waitStages;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &m_CommandBuffers[currentFrame];

			VkSemaphore signalSemaphores[] = {signalSemaphore };
			submitInfo.signalSemaphoreCount = 1;
			submitInfo.pSignalSemaphores = signalSemaphores;

			res = vkQueueSubmit(context->getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
			REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to submit draw command buffer");
		}
	}

	void UnlitPass::resize(const VulkanContext* context, uint width, uint height, std::vector<VkImageView> endImageViews)
	{
		m_Width = width;
		m_Height = height;
		for (auto& framebuffer : m_Framebuffers) {
			vkDestroyFramebuffer(context->getDevice(), framebuffer, nullptr);
		}
		m_Framebuffers.clear();
		vkDestroyImageView(context->getDevice(), m_ColorImageView, nullptr);
		vkDestroyImage(context->getDevice(), m_ColorImage, nullptr);
		vmaFreeMemory(context->getAllocator(), m_ColorImageAllocation);
		vkDestroyImageView(context->getDevice(), m_DepthImageView, nullptr);
		vkDestroyImage(context->getDevice(), m_DepthImage, nullptr);
		vmaFreeMemory(context->getAllocator(), m_DepthImageAllocation);

		createImages(context);
		createFrameBuffers(context, endImageViews);
	}

	void UnlitPass::createImages(const VulkanContext* context) {
		context->createImage(m_Width, m_Height, 1, context->getSampleCount(), VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, m_ColorImage, m_ColorImageAllocation);
		m_ColorImageView = context->createImageView(m_ColorImage, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, 1);

		context->createImage(m_Width, m_Height, 1, context->getSampleCount(), context->findDepthFormat(), VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, m_DepthImage, m_DepthImageAllocation);
		m_DepthImageView = context->createImageView(m_DepthImage, context->findDepthFormat(), VK_IMAGE_ASPECT_DEPTH_BIT, 1);
	}

	VkPipeline UnlitPass::getPipelineFromFlags(const VulkanContext* context, uint32_t flags)
	{
		if (m_PipelineByMaterialPermutation.contains(flags)) {
			return m_PipelineByMaterialPermutation[flags];
		}
		else {
			VkPipeline newPipeline = createGraphicsPipeline(context, m_PipelineCache, flags);
			m_PipelineByMaterialPermutation[flags] = newPipeline;
			return newPipeline;
		}
	}

	void UnlitPass::createRenderPass(const VulkanContext* context) {
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = VK_FORMAT_R16G16B16A16_SFLOAT;
		colorAttachment.samples = context->getSampleCount();
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription depthAttachment{};
		depthAttachment.format = context->findDepthFormat();
		depthAttachment.samples = context->getSampleCount();
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription colorResolveAttachment{};
		colorResolveAttachment.format = VK_FORMAT_R16G16B16A16_SFLOAT;
		colorResolveAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorResolveAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorResolveAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorResolveAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorResolveAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef{};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colorResolveAttachmentRef{};
		colorResolveAttachmentRef.attachment = 2;
		colorResolveAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		std::array<VkAttachmentReference, 1> colorRefs = { colorAttachmentRef };
		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = colorRefs.data();
		subpass.pResolveAttachments = &colorResolveAttachmentRef;
		subpass.pDepthStencilAttachment = &depthAttachmentRef;

		std::array<VkAttachmentDescription, 3> attachments = { colorAttachment, depthAttachment, colorResolveAttachment };
		VkRenderPassCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		createInfo.attachmentCount = 3;
		createInfo.pAttachments = attachments.data();
		createInfo.subpassCount = 1;
		createInfo.pSubpasses = &subpass;

		VkResult res = vkCreateRenderPass(context->getDevice(), &createInfo, nullptr, &m_RenderPass);
		REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to create render pass");
	}

	void UnlitPass::createFrameBuffers(const VulkanContext* context, std::vector<VkImageView> endImageViews) {
		m_Framebuffers.resize(context->getSwapChainImageViews().size());

		for (size_t i = 0; i < context->getSwapChainImageViews().size(); i++) {
			std::array<VkImageView, 3> attachments = {
				m_ColorImageView,
				m_DepthImageView,
				endImageViews[i]
			};

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = m_RenderPass;
			framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			framebufferInfo.pAttachments = attachments.data();
			framebufferInfo.width = m_Width;
			framebufferInfo.height = m_Height;
			framebufferInfo.layers = 1;

			VkResult res = vkCreateFramebuffer(context->getDevice(), &framebufferInfo, nullptr, &m_Framebuffers[i]);
			REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to create framebuffer");
		}
	}

	VkPipeline UnlitPass::createGraphicsPipeline(const VulkanContext* context, VkPipelineCache pipelineCache, uint32_t flags) {

		auto vertShaderCode = Shader::CompileHLSLToSPIRV("Assets/Shaders/Unlit.vert", flags);
		auto fragShaderCode = Shader::CompileHLSLToSPIRV("Assets/Shaders/Unlit.frag", flags);

		if (vertShaderCode.empty() || fragShaderCode.empty()) {
			REON_CORE_ERROR("Shader code not compiled correctly");
			return VK_NULL_HANDLE;
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
		multisampling.sampleShadingEnable = VK_TRUE;
		multisampling.rasterizationSamples = context->getSampleCount();
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
			VK_DYNAMIC_STATE_SCISSOR,
			VK_DYNAMIC_STATE_CULL_MODE,
			VK_DYNAMIC_STATE_POLYGON_MODE_EXT
		};

		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = m_DescriptorSetLayouts.size();
		pipelineLayoutInfo.pSetLayouts = m_DescriptorSetLayouts.data();
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

		VkPipeline graphicsPipeline = VK_NULL_HANDLE;

		res = vkCreateGraphicsPipelines(context->getDevice(), pipelineCache, 1, &pipelineInfo, nullptr, &graphicsPipeline);
		REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to create graphics pipeline")

		vkDestroyShaderModule(context->getDevice(), fragShaderModule, nullptr);
		vkDestroyShaderModule(context->getDevice(), vertShaderModule, nullptr);

		return graphicsPipeline;
	}
}