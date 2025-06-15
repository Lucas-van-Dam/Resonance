#include "reonpch.h"
#include "RenderManager.h"

#include "REON/GameHierarchy/Components/Transform.h"
#include "REON/GameHierarchy/GameObject.h"
#include "stb_image_wrapper.h"
#include "REON/Application.h"
#include "REON/EditorCamera.h"
#include <REON/KeyCodes.h>

namespace REON {
	uint RenderManager::QuadVAO;
	std::shared_ptr<BloomEffect> RenderManager::m_BloomEffect;
	std::shared_ptr<ColorCorrection> RenderManager::m_ColorCorrection;
	std::shared_ptr<DepthOfField> RenderManager::m_DepthOfField;

	void RenderManager::Render() {
		resized = false;
		prepareDrawCommands(m_Context->getCurrentFrame());
		if (m_DrawCommandsByShaderMaterial.empty()) {
			VkSemaphore signalSemaphores[] = { m_Context->getCurrentRenderFinishedSemaphore() };
			VkSemaphore waitSemaphores[] = { m_Context->getCurrentImageAvailableSemaphore(), m_DirectionalShadowsGenerated[m_Context->getCurrentFrame()] };
			VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
			VkSubmitInfo submitInfo{};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.signalSemaphoreCount = 1;
			submitInfo.waitSemaphoreCount = 2;
			submitInfo.pSignalSemaphores = signalSemaphores;
			submitInfo.pWaitSemaphores = waitSemaphores;
			submitInfo.pWaitDstStageMask = waitStages;

			vkQueueSubmit(m_Context->getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
			return;
		}
		setGlobalData();

		if (renderMode != LIT) {
			m_UnlitPass.render(m_Context, m_DrawCommandsByShaderMaterial, m_GlobalDescriptorSets[m_Context->getCurrentImageIndex()], m_Context->getCurrentRenderFinishedSemaphore(), renderMode);
			return;
		}
		GenerateShadows();
		RenderOpaques();
		RenderTransparents();
	}

	void RenderManager::AddRenderer(const std::shared_ptr<Renderer>& renderer) {
		m_Renderers.push_back(renderer);
		createOpaqueObjectDescriptorSets(renderer);
	}

	void RenderManager::RemoveRenderer(std::shared_ptr<Renderer> renderer)
	{
		REON_CORE_WARN("Removing renderer from object: {0}", renderer->GetOwner()->GetName());
		REON_CORE_WARN("Renderer use count: {0}", renderer.use_count());
		REON_CORE_WARN("Renderers size: {0}", m_Renderers.size());
		m_Renderers.erase(std::remove(m_Renderers.begin(), m_Renderers.end(), renderer), m_Renderers.end());
		REON_CORE_INFO("Succesfully Removed renderer from object: {0}", renderer->GetOwner()->GetName());
	}

	void RenderManager::RenderSkyBox() {
	}

	void RenderManager::RenderOpaques() {			
		int currentFrame = m_Context->getCurrentFrame();
		int currentImageIndex = m_Context->getCurrentImageIndex();

		for (const auto& pair : m_DrawCommandsByShaderMaterial) {
			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = 0;
			beginInfo.pInheritanceInfo = nullptr;

			VkResult res = vkBeginCommandBuffer(m_OpaqueCommandBuffers[currentFrame], &beginInfo);
			REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to begin recording command buffer");

			VkRenderPassBeginInfo renderPassInfo{};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = m_OpaqueRenderPass;
			renderPassInfo.framebuffer = m_OpaqueFramebuffers[m_Context->getCurrentImageIndex()];
			renderPassInfo.renderArea.offset = { 0,0 };
			renderPassInfo.renderArea.extent = {m_Width, m_Height};

			std::array<VkClearValue, 4> clearValues{};
			clearValues[0].color = { 0.1f, 0.1f, 0.1f, 1.0f };
			clearValues[1].depthStencil = { 1.0f, 0 };
			clearValues[2].color = { 0.1f, 0.1f, 0.1f, 1.0f };
			clearValues[3].depthStencil = { 1.0f, 0 };
			renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
			renderPassInfo.pClearValues = clearValues.data();

			VkImageMemoryBarrier barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = m_OpaqueResolveImages[currentImageIndex];
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = 1;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 1;
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;

			vkCmdPipelineBarrier(m_OpaqueCommandBuffers[currentFrame],
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier);

			vkCmdBeginRenderPass(m_OpaqueCommandBuffers[currentFrame], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);




			const auto& materialFromShader = pair.second;
			for (const auto& material : materialFromShader) {
				//set material wide buffers/textures
				std::shared_ptr<Material> mat = ResourceManager::GetInstance().GetResource<Material>(material.first);
				if (!(mat->blendingMode == Mask || mat->renderingMode == Opaque)) {
					continue;
				}
				auto pipeline = getPipelineFromFlags(mat->materialFlags);
				if (pipeline == VK_NULL_HANDLE) {
					REON_CORE_WARN("Cant render because pipeline is not found");
					continue;
				}
				vkCmdBindPipeline(m_OpaqueCommandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

				VkViewport viewport{};
				viewport.x = 0.0f;
				viewport.y = 0.0f;
				viewport.width = static_cast<float>(m_Width);
				viewport.height = static_cast<float>(m_Height);
				viewport.minDepth = 0.0f;
				viewport.maxDepth = 1.0f;
				vkCmdSetViewport(m_OpaqueCommandBuffers[currentFrame], 0, 1, &viewport);

				VkRect2D scissor{};
				scissor.offset = { 0,0 };
				scissor.extent = { m_Width, m_Height };
				vkCmdSetScissor(m_OpaqueCommandBuffers[currentFrame], 0, 1, &scissor);

				memcpy(mat->flatDataBuffersMapped[currentFrame], &mat->flatData, sizeof(mat->flatData));

				vkCmdSetCullMode(m_OpaqueCommandBuffers[currentFrame], mat->getDoubleSided() ? VK_CULL_MODE_NONE : VK_CULL_MODE_BACK_BIT);


				for (const auto& cmd : material.second) {
					std::vector<VkDescriptorSet> descriptorSets = { m_GlobalDescriptorSets[currentFrame], mat->descriptorSets[currentFrame], cmd.owner->objectDescriptorSets[m_Context->getCurrentFrame()]};

					ObjectRenderData data{};
					data.model = cmd.owner->getModelMatrix();
					data.transposeInverseModel = cmd.owner->getTransposeInverseModelMatrix();
					memcpy(cmd.owner->objectDataBuffersMapped[m_Context->getCurrentFrame()], &data, sizeof(data));

					VkBuffer vertexBuffers[] = { cmd.mesh->m_VertexBuffer };
					VkDeviceSize offsets[] = { 0 };
					vkCmdBindVertexBuffers(m_OpaqueCommandBuffers[currentFrame], 0, 1, vertexBuffers, offsets);

					vkCmdBindIndexBuffer(m_OpaqueCommandBuffers[currentFrame], cmd.mesh->m_IndexBuffer, 0, VK_INDEX_TYPE_UINT32);

					vkCmdBindDescriptorSets(m_OpaqueCommandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS, m_OpaquePipelineLayout, 0, descriptorSets.size(), descriptorSets.data(), 0, nullptr);
					vkCmdDrawIndexed(m_OpaqueCommandBuffers[currentFrame], static_cast<uint32_t>(cmd.indexCount), 1, cmd.startIndex, 0, 0);
				}
			}

			vkCmdEndRenderPass(m_OpaqueCommandBuffers[currentFrame]);

			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = m_OpaqueResolveImages[currentImageIndex];
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = 1;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 1;
			barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(m_OpaqueCommandBuffers[currentFrame],
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier);

			res = vkEndCommandBuffer(m_OpaqueCommandBuffers[currentFrame]);
			REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to record command buffer");

			VkSubmitInfo submitInfo{};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

			VkSemaphore waitSemaphores[] = { m_Context->getCurrentImageAvailableSemaphore(), m_DirectionalShadowsGenerated[currentFrame]};
			VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT };
			submitInfo.waitSemaphoreCount = 2;
			submitInfo.pWaitSemaphores = waitSemaphores;
			submitInfo.pWaitDstStageMask = waitStages;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &m_OpaqueCommandBuffers[currentFrame];

			VkSemaphore signalSemaphores[] = { m_OpaquePassDone[currentFrame]};
			submitInfo.signalSemaphoreCount = 1;
			submitInfo.pSignalSemaphores = signalSemaphores;

			res = vkQueueSubmit(m_Context->getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
			REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to submit draw command buffer");
		}
	}

	void RenderManager::RenderTransparents() {
		int currentFrame = m_Context->getCurrentFrame();
		m_TransparentPass.render(m_Context, m_DrawCommandsByShaderMaterial, m_OpaquePassDone[currentFrame], m_Context->getCurrentRenderFinishedSemaphore(),
			m_GlobalDescriptorSets[m_Context->getCurrentImageIndex()]);
	}

	void RenderManager::RenderPostProcessing() {
		m_RenderResultTexture = m_PostProcessingStack.Render(m_SceneTexture, m_SceneDepthTex);
	}

	void RenderManager::GenerateShadows() {
		auto light = m_LightManager->mainLight;
		glm::mat4 lightSpaceMatrix;
		float near_plane = -50.0f, far_plane = 100;
		m_MainLightProj = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, near_plane, far_plane);
		//m_MainLightProj[1][1] *= -1.0f;
		m_MainLightView = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), (light->GetOwner()->GetTransform()->localRotation * glm::vec3(0.0f, 0.0f, -1.0f)), (light->GetOwner()->GetTransform()->localRotation * glm::vec3(0.0f, 1.0f, 0.0f)));
		lightSpaceMatrix = m_MainLightProj * m_MainLightView;
		m_DirectionalShadowPass.render(m_Context, m_Renderers, lightSpaceMatrix, m_DirectionalShadowsGenerated[m_Context->getCurrentFrame()]);
		return;
		GenerateMainLightShadows();
		GenerateAdditionalShadows();
	}

	RenderManager::RenderManager(std::shared_ptr<LightManager> lightManager, std::shared_ptr<EditorCamera> camera) : m_LightManager(std::move(lightManager)), m_Context(static_cast<const VulkanContext*>(Application::Get().GetRenderContext())) {
		m_Width = Application::Get().GetWindow().GetWidth();
		m_Height = Application::Get().GetWindow().GetHeight();

		m_KeyPressedCallbackID = EventBus::Get().subscribe<KeyPressedEvent>(REON_BIND_EVENT_FN(RenderManager::OnKeyPressed));
		//EventBus::Get().subscribe<MaterialChangedEvent>(REON_BIND_EVENT_FN(RenderManager::OnMaterialChanged));

		//Initialize main light shadow maps
		this->m_Camera = std::move(camera);

		m_DirectionalShadowPass.Init(m_Context);

		//m_DrawCommandsByShaderMaterial.resize(m_Context->MAX_FRAMES_IN_FLIGHT);

		createPipelineCache();
		createGlobalBuffers();
		createDummyResources();
		createOpaqueCommandPool();
		createOpaqueCommandBuffers();
		createOpaqueImages();
		createOpaqueRenderPass();
		createOpaqueFrameBuffers();
		createOpaqueDescriptorSetLayout();
		createOpaqueGraphicsPipelines();
		createEndBufferSet();
		createSyncObjects();
		createOpaqueGlobalDescriptorSets();

		std::vector<VkDescriptorSetLayout> layouts;
		layouts.push_back(m_OpaqueGlobalDescriptorSetLayout);
		layouts.push_back(m_OpaqueMaterialDescriptorSetLayout);
		layouts.push_back(m_OpaqueObjectDescriptorSetLayout);
		m_TransparentPass.init(m_Context, m_Width, m_Height, layouts, m_PipelineCache, m_OpaqueResolveImageViews, m_EndImageViews, m_DepthResolveViews);
		m_UnlitPass.init(m_Context, m_Width, m_Height, m_EndImageViews, m_PipelineCache, layouts);
	}

	void RenderManager::RenderFullScreenQuad()
	{
	}

	void RenderManager::OnKeyPressed(const KeyPressedEvent& event)
	{
		if (event.GetKeyCode() == REON_KEY_I && event.GetRepeatCount() == 0) {
			//m_PostProcessingStack.StartProfiling();
		}
		if (event.GetKeyCode() == REON_KEY_O && event.GetRepeatCount() == 0) {
			//m_PostProcessingStack.ExportFrameDataToCSV("BloomFrameData.csv", "Bloom");
		}
		if (event.GetKeyCode() == REON_KEY_K && event.GetRepeatCount() == 0) {
			//m_PostProcessingStack.ExportFrameDataToCSV("DepthOfFieldFrameData.csv", "Depth of Field");
		}
	}

	void RenderManager::GenerateMainLightShadows() {
	}

	void RenderManager::GenerateAdditionalShadows() {
		if (m_PointLights.size() == 0)
			Initialize();
		for (int i = 0; i < m_PointLights.size(); ++i) {
			std::shared_ptr<Light> light = m_PointLights[i];
			glm::vec3 lightPos = light->GetOwner()->GetTransform()->localPosition;
			float near_plane = 1.0f;
			float far_plane = 100.0f;
			glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), (float)ADDITIONAL_SHADOW_WIDTH / (float)ADDITIONAL_SHADOW_HEIGHT, near_plane, far_plane);
			std::vector<glm::mat4> shadowTransforms;
			shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
			shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
			shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
			shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)));
			shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
			shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
			glViewport(0, 0, ADDITIONAL_SHADOW_WIDTH, ADDITIONAL_SHADOW_HEIGHT);
			glBindFramebuffer(GL_FRAMEBUFFER, m_AdditionalDepthFBOs[i]);
			glClear(GL_DEPTH_BUFFER_BIT);
			glDisable(GL_CULL_FACE);
			m_AdditionalShadowShader->use();
			for (unsigned int j = 0; j < 6; ++j)
				//m_AdditionalShadowShader->setMat4("shadowMatrices[" + std::to_string(j) + "]", shadowTransforms[j]);
			//m_AdditionalShadowShader->setFloat("far_plane", far_plane);
			//m_AdditionalShadowShader->setVec3("lightPos", lightPos);
				for (const auto& renderer : m_Renderers) {
					renderer->Draw(glm::mat4(), glm::mat4(), -1, -1, -1, -1, std::vector<int>(), 0, m_AdditionalShadowShader);
				}
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glEnable(GL_CULL_FACE);
			glViewport(0, 0, m_Width, m_Height);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}
	}

	void RenderManager::Initialize() {
		return;
		for (const std::shared_ptr<Light>& light : m_LightManager->lights) {
			if (light->type == LightType::Point) {
				m_PointLights.emplace_back(light);
			}
		}
	}

	void RenderManager::InitializeSkyBox()
	{

	}

	std::vector<LightData> RenderManager::GetLightingBuffer() {
		const auto& scene = SceneManager::Get()->GetCurrentScene();
		size_t amountOfLights = scene->lightManager->lights.size();
		int pointIndex = 0;
		std::vector<LightData> lights;

		glm::mat4 lightSpaceMatrix;
		float near_plane = -50.0f, far_plane = 100;
		glm::mat4 mainLightProj = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, near_plane, far_plane);
		//mainLightProj[1][1] *= -1.0f;
		glm::mat4 mainLightView = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), (scene->lightManager->lights[0]->GetOwner()->GetTransform()->localRotation * glm::vec3(0.0f, 0.0f, -1.0f)), (scene->lightManager->lights[0]->GetOwner()->GetTransform()->localRotation * glm::vec3(0.0f, 1.0f, 0.0f)));
		lightSpaceMatrix = mainLightProj * mainLightView;

		for (int i = 0; i < amountOfLights; ++i) {
			unsigned int depthCube = -1;
			Light* light = scene->lightManager->lights[i].get();

			LightData data(light->intensity, light->color, light->GetOwner()->GetTransform()->localPosition,
				light->GetOwner()->GetTransform()->GetForwardVector(), light->innerCutOff, light->outerCutOff, (int)light->type, lightSpaceMatrix);
			lights.emplace_back(data);
		}
		return lights;
	}

	void RenderManager::setGlobalData()
	{
		const auto& viewMat = m_Camera->GetViewMatrix();
		const auto& projMat = m_Camera->GetProjectionMatrix();

		GlobalRenderData data{};
		auto lightData = GetLightingBuffer();

		std::vector<LightData> lights;

		data.viewProj = projMat * viewMat;
		data.inverseView = glm::inverse(viewMat);
		for (int i = 0; i < lightData.size(); i++) {
			if (i >= REON_MAX_LIGHTS) {
				REON_CORE_WARN("More lights in scene than allowed ({}), ignoring further lights", lightData.size());
				break;
			}
			lights.push_back(lightData[i]);
		}
		data.lightCount = glm::min(static_cast<int>(lightData.size()), REON_MAX_LIGHTS);

		memcpy(m_GlobalDataBuffersMapped[m_Context->getCurrentImageIndex()], &data, sizeof(data));

		memcpy(m_LightDataBuffersMapped[m_Context->getCurrentImageIndex()], lights.data(), lights.size() * sizeof(LightData));
	}

	void RenderManager::prepareDrawCommands(int currentFrame)
	{
		m_DrawCommandsByShaderMaterial.clear();
		for (auto& renderer : m_Renderers) {
			if (renderer->drawCommandsDirty)
				renderer->RebuildDrawCommands();
			
			for (DrawCommand& cmd : renderer->drawCommands) {
				auto shaderID = cmd.shader->GetID();
				auto materialID = cmd.material->GetID();

				if (materials.find(materialID) == materials.end()) {
					materials.insert(materialID);
					createOpaqueMaterialDescriptorSets(*cmd.material);
				}

				m_DrawCommandsByShaderMaterial[shaderID][materialID].push_back(cmd);
			}
		}
	}

	void RenderManager::createSyncObjects()
	{
		m_DirectionalShadowsGenerated.resize(m_Context->MAX_FRAMES_IN_FLIGHT);
		m_OpaquePassDone.resize(m_Context->MAX_FRAMES_IN_FLIGHT);

		VkSemaphoreCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		for (int i = 0; i < m_Context->MAX_FRAMES_IN_FLIGHT; i++) {
			vkCreateSemaphore(m_Context->getDevice(), &createInfo, nullptr, &m_DirectionalShadowsGenerated[i]);
			vkCreateSemaphore(m_Context->getDevice(), &createInfo, nullptr, &m_OpaquePassDone[i]);
		}
	}

	void RenderManager::createDummyResources()
	{
		int texWidth, texHeight, texChannels;
		stbi_uc* pixels = stbi_load("Assets/Textures/1x1White.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		VkDeviceSize imageSize = texWidth * texHeight * 4;

		REON_CORE_ASSERT(pixels, "Failed to load texture image");

		VkBuffer stagingBuffer;
		VmaAllocation stagingBufferAllocation;

		m_Context->createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, stagingBuffer, stagingBufferAllocation);

		void* data;
		vmaMapMemory(m_Context->getAllocator(), stagingBufferAllocation, &data);
		memcpy(data, pixels, static_cast<size_t>(imageSize));
		vmaUnmapMemory(m_Context->getAllocator(), stagingBufferAllocation);

		stbi_image_free(pixels);

		m_Context->createImage(texWidth, texHeight, 1, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, m_DummyImage, m_DummyImageAllocation);

		m_Context->transitionImageLayout(m_DummyImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1);
		m_Context->copyBufferToImage(stagingBuffer, m_DummyImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

		//generateMipmaps(m_TextureImage, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, m_MipLevels);
		m_Context->transitionImageLayout(m_DummyImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);

		vkDestroyBuffer(m_Context->getDevice(), stagingBuffer, nullptr);
		vmaFreeMemory(m_Context->getAllocator(), stagingBufferAllocation);

		m_DummyImageView = m_Context->createImageView(m_DummyImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, 1);

		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;


		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(m_Context->getPhysicalDevice(), &properties); // TODO: IF I USE THIS MORE STORE IT AT THE START AT CREATION

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

		VkResult res = vkCreateSampler(m_Context->getDevice(), &samplerInfo, nullptr, &m_DummySampler);
		REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to create texture sampler");
	}

	void RenderManager::createEndBufferSet()
	{
		VkDescriptorSetLayoutBinding endLayoutBinding{};
		endLayoutBinding.binding = 0;
		endLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		endLayoutBinding.descriptorCount = 1;
		endLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		endLayoutBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = 1;
		layoutInfo.pBindings = &endLayoutBinding;

		VkResult res = vkCreateDescriptorSetLayout(m_Context->getDevice(), &layoutInfo, nullptr, &m_EndDescriptorSetLayout);
		REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to create descriptor set layout");

		std::vector<VkDescriptorSetLayout> endLayouts(m_Context->getAmountOfSwapChainImages(), m_EndDescriptorSetLayout);
		VkDescriptorSetAllocateInfo endAllocInfo{};
		endAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		endAllocInfo.descriptorPool = m_Context->getDescriptorPool();
		endAllocInfo.descriptorSetCount = static_cast<uint32_t>(m_Context->getAmountOfSwapChainImages());
		endAllocInfo.pSetLayouts = endLayouts.data();

		m_EndDescriptorSets.resize(m_Context->getAmountOfSwapChainImages());

		res = vkAllocateDescriptorSets(m_Context->getDevice(), &endAllocInfo, m_EndDescriptorSets.data());
		REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to allocate descriptor sets");

		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(m_Context->getPhysicalDevice(), &properties); // TODO: IF I USE THIS MORE STORE IT AT THE START AT CREATION

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

		res = vkCreateSampler(m_Context->getDevice(), &samplerInfo, nullptr, &m_EndSampler);
		REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to create texture sampler");
		

		for (size_t i = 0; i < m_Context->getAmountOfSwapChainImages(); i++) {
			VkDescriptorImageInfo endImageInfo{};
			endImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			endImageInfo.imageView = m_EndImageViews[i];
			endImageInfo.sampler = m_EndSampler;

			std::array<VkWriteDescriptorSet, 1> descriptorWrites{};
			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = m_EndDescriptorSets[i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pImageInfo = &endImageInfo;

			vkUpdateDescriptorSets(m_Context->getDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}
	}

	void RenderManager::createOpaqueCommandPool()
	{
		QueueFamilyIndices queueFamilyIndices = m_Context->findQueueFamilies(m_Context->getPhysicalDevice());

		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

		VkResult res = vkCreateCommandPool(m_Context->getDevice(), &poolInfo, nullptr, &m_OpaqueCommandPool);
		REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to create command pool");
	}

	void RenderManager::createOpaqueCommandBuffers()
	{
		m_OpaqueCommandBuffers.resize(m_Context->MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = m_OpaqueCommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = static_cast<uint32_t>(m_OpaqueCommandBuffers.size());

		VkResult res = vkAllocateCommandBuffers(m_Context->getDevice(), &allocInfo, m_OpaqueCommandBuffers.data());
		REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to allocate command buffers");
	}

	void RenderManager::createOpaqueImages()
	{
		m_Context->createImage(m_Width, m_Height, 1, m_Context->getSampleCount(), VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, m_ColorImage, m_ColorImageAllocation);
		m_ColorImageView = m_Context->createImageView(m_ColorImage, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, 1);

		m_Context->createImage(m_Width, m_Height, 1, m_Context->getSampleCount(), m_Context->findDepthFormat(), VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, m_DepthImage, m_DepthImageAllocation);
		m_DepthImageView = m_Context->createImageView(m_DepthImage, m_Context->findDepthFormat(), VK_IMAGE_ASPECT_DEPTH_BIT, 1);

		size_t swapChainImageCount = m_Context->getAmountOfSwapChainImages();

		m_OpaqueResolveImages.resize(swapChainImageCount);
		m_OpaqueResolveImageViews.resize(swapChainImageCount);
		m_OpaqueResolveImageAllocations.resize(swapChainImageCount);

		for (int i = 0; i < swapChainImageCount; i++) {
			m_Context->createImage(m_Width, m_Height, 1, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, m_OpaqueResolveImages[i], m_OpaqueResolveImageAllocations[i]);
			m_OpaqueResolveImageViews[i] = m_Context->createImageView(m_OpaqueResolveImages[i], VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, 1);
		}

		m_EndImages.resize(swapChainImageCount);
		m_EndImageViews.resize(swapChainImageCount);
		m_EndImageAllocations.resize(swapChainImageCount);

		for (int i = 0; i < swapChainImageCount; i++) {
			m_Context->createImage(m_Width, m_Height, 1, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, m_EndImages[i], m_EndImageAllocations[i]);
			m_EndImageViews[i] = m_Context->createImageView(m_EndImages[i], VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, 1);
		}

		m_DepthResolveImages.resize(swapChainImageCount);
		m_DepthResolveViews.resize(swapChainImageCount);
		m_DepthResolveAllocations.resize(swapChainImageCount);

		for (int i = 0; i < swapChainImageCount; i++) {
			m_Context->createImage(m_Width, m_Height, 1, VK_SAMPLE_COUNT_1_BIT, m_Context->findDepthFormat(), VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, m_DepthResolveImages[i], m_DepthResolveAllocations[i]);
			m_DepthResolveViews[i] = m_Context->createImageView(m_DepthResolveImages[i], m_Context->findDepthFormat(), VK_IMAGE_ASPECT_DEPTH_BIT, 1);
		}
	}

	void RenderManager::createOpaqueRenderPass()
	{
		VkAttachmentDescription2 colorAttachment{};
		colorAttachment.format = VK_FORMAT_R16G16B16A16_SFLOAT;
		colorAttachment.samples = m_Context->getSampleCount();
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorAttachment.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;

		VkAttachmentDescription2 depthAttachment{};
		depthAttachment.format = m_Context->findDepthFormat();
		depthAttachment.samples = m_Context->getSampleCount();
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		depthAttachment.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;

		VkAttachmentDescription2 colorAttachmentResolve{};
		colorAttachmentResolve.format = VK_FORMAT_R16G16B16A16_SFLOAT;
		colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorAttachmentResolve.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;

		VkAttachmentDescription2 depthAttachmentResolve{};
		depthAttachmentResolve.format = m_Context->findDepthFormat();
		depthAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		depthAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
		depthAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		depthAttachmentResolve.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;

		VkAttachmentReference2 colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorAttachmentRef.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;

		VkAttachmentReference2 depthAttachmentRef{};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		depthAttachmentRef.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;

		VkAttachmentReference2 colorAttachmentResolveRef{};
		colorAttachmentResolveRef.attachment = 2;
		colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorAttachmentResolveRef.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;

		VkAttachmentReference2 depthAttachmentResolveRef{};
		depthAttachmentResolveRef.attachment = 3;
		depthAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
		depthAttachmentResolveRef.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;

		VkSubpassDependency2 dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependency.sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2;

		VkSubpassDescriptionDepthStencilResolve depthResolve = {};
		depthResolve.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_DEPTH_STENCIL_RESOLVE;
		depthResolve.pDepthStencilResolveAttachment = &depthAttachmentResolveRef;
		depthResolve.depthResolveMode = VK_RESOLVE_MODE_SAMPLE_ZERO_BIT;
		depthResolve.stencilResolveMode = VK_RESOLVE_MODE_NONE;

		VkSubpassDescription2 subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		subpass.pDepthStencilAttachment = &depthAttachmentRef;
		subpass.pResolveAttachments = &colorAttachmentResolveRef;
		subpass.pNext = &depthResolve;
		subpass.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2;
	

		std::array<VkAttachmentDescription2, 4> attachments = { colorAttachment, depthAttachment, colorAttachmentResolve, depthAttachmentResolve };
		VkRenderPassCreateInfo2 renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		VkResult res = vkCreateRenderPass2(m_Context->getDevice(), &renderPassInfo, nullptr, &m_OpaqueRenderPass);
		REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to create render pass")
	}

	void RenderManager::createOpaqueFrameBuffers()
	{
		m_OpaqueFramebuffers.resize(m_Context->getSwapChainImageViews().size());

		for (size_t i = 0; i < m_Context->getSwapChainImageViews().size(); i++) {
			std::array<VkImageView, 4> attachments = {
				m_ColorImageView,
				m_DepthImageView,
				m_OpaqueResolveImageViews[i],
				m_DepthResolveViews[i]
			};

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = m_OpaqueRenderPass;
			framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			framebufferInfo.pAttachments = attachments.data();
			framebufferInfo.width = m_Width;
			framebufferInfo.height = m_Height;
			framebufferInfo.layers = 1;

			VkResult res = vkCreateFramebuffer(m_Context->getDevice(), &framebufferInfo, nullptr, &m_OpaqueFramebuffers[i]);
			REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to create framebuffer");
		}
	}

	void RenderManager::createOpaqueDescriptorSetLayout()
	{
		VkDescriptorSetLayoutBinding globalLayoutBinding{};
		globalLayoutBinding.binding = 0;
		globalLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		globalLayoutBinding.descriptorCount = 1;
		globalLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		globalLayoutBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding globalLightBinding{};
		globalLightBinding.binding = 1;
		globalLightBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		globalLightBinding.descriptorCount = 1;
		globalLightBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		globalLightBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding globalDirectionalShadowBinding{};
		globalDirectionalShadowBinding.binding = 2;
		globalDirectionalShadowBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		globalDirectionalShadowBinding.descriptorCount = 1;
		globalDirectionalShadowBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		globalDirectionalShadowBinding.pImmutableSamplers = nullptr;

		std::array<VkDescriptorSetLayoutBinding, 3> globalBindings{ globalLayoutBinding, globalLightBinding, globalDirectionalShadowBinding };
		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = globalBindings.size();
		layoutInfo.pBindings = globalBindings.data();

		VkResult res = vkCreateDescriptorSetLayout(m_Context->getDevice(), &layoutInfo, nullptr, &m_OpaqueGlobalDescriptorSetLayout);
		REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to create descriptor set layout");

		VkDescriptorSetLayoutBinding flatDataBinding{};
		flatDataBinding.binding = 1;
		flatDataBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		flatDataBinding.descriptorCount = 1;
		flatDataBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		flatDataBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutBinding albedoLayoutBinding{};
		albedoLayoutBinding.binding = 3;
		albedoLayoutBinding.descriptorCount = 1;
		albedoLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		albedoLayoutBinding.pImmutableSamplers = nullptr;
		albedoLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutBinding normalLayoutBinding{};
		normalLayoutBinding.binding = 4;
		normalLayoutBinding.descriptorCount = 1;
		normalLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		normalLayoutBinding.pImmutableSamplers = nullptr;
		normalLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutBinding roughnessMetallicLayoutBinding{};
		roughnessMetallicLayoutBinding.binding = 5;
		roughnessMetallicLayoutBinding.descriptorCount = 1;
		roughnessMetallicLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		roughnessMetallicLayoutBinding.pImmutableSamplers = nullptr;
		roughnessMetallicLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutBinding emissiveLayoutBinding{};
		emissiveLayoutBinding.binding = 6;
		emissiveLayoutBinding.descriptorCount = 1;
		emissiveLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		emissiveLayoutBinding.pImmutableSamplers = nullptr;
		emissiveLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutBinding specularLayoutBinding{};
		specularLayoutBinding.binding = 7;
		specularLayoutBinding.descriptorCount = 1;
		specularLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		specularLayoutBinding.pImmutableSamplers = nullptr;
		specularLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutBinding specularColorLayoutBinding{};
		specularColorLayoutBinding.binding = 8;
		specularColorLayoutBinding.descriptorCount = 1;
		specularColorLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		specularColorLayoutBinding.pImmutableSamplers = nullptr;
		specularColorLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		std::array<VkDescriptorSetLayoutBinding, 7> bindings = { flatDataBinding, albedoLayoutBinding, normalLayoutBinding, roughnessMetallicLayoutBinding, emissiveLayoutBinding, specularLayoutBinding, specularColorLayoutBinding };
		VkDescriptorSetLayoutCreateInfo materialLayoutInfo{};
		materialLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		materialLayoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		materialLayoutInfo.pBindings = bindings.data();
		//layoutInfo.flags |= VK_DESCRIPTOR_SET_LAYOUT_CREATE_PER_STAGE_BIT_NV;

		res = vkCreateDescriptorSetLayout(m_Context->getDevice(), &materialLayoutInfo, nullptr, &m_OpaqueMaterialDescriptorSetLayout);
		REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to create descriptor set layout");

		VkDescriptorSetLayoutBinding objectDataBinding{};
		objectDataBinding.binding = 2;
		objectDataBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		objectDataBinding.descriptorCount = 1;
		objectDataBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		objectDataBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutCreateInfo objectLayoutInfo{};
		objectLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		objectLayoutInfo.bindingCount = 1;
		objectLayoutInfo.pBindings = &objectDataBinding;

		res = vkCreateDescriptorSetLayout(m_Context->getDevice(), &objectLayoutInfo, nullptr, &m_OpaqueObjectDescriptorSetLayout);
		REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to create descriptor set layout");
	}

	void RenderManager::createOpaqueGlobalDescriptorSets()
	{
		m_GlobalDescriptorSets.resize(m_Context->getAmountOfSwapChainImages());

		std::vector<VkDescriptorSetLayout> globalLayouts(m_Context->getAmountOfSwapChainImages(), m_OpaqueGlobalDescriptorSetLayout);
		VkDescriptorSetAllocateInfo globalAllocInfo{};
		globalAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		globalAllocInfo.descriptorPool = m_Context->getDescriptorPool();
		globalAllocInfo.descriptorSetCount = static_cast<uint32_t>(m_Context->getAmountOfSwapChainImages());
		globalAllocInfo.pSetLayouts = globalLayouts.data();

		VkResult res = vkAllocateDescriptorSets(m_Context->getDevice(), &globalAllocInfo, m_GlobalDescriptorSets.data());
		REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to allocate descriptor sets");

		for (size_t i = 0; i < m_Context->getAmountOfSwapChainImages(); i++) {
			VkDescriptorBufferInfo globalBufferInfo{};
			globalBufferInfo.buffer = m_GlobalDataBuffers[i];
			globalBufferInfo.offset = 0;
			globalBufferInfo.range = sizeof(GlobalRenderData);

			VkDescriptorBufferInfo lightBufferInfo{};
			lightBufferInfo.buffer = m_LightDataBuffers[i];
			lightBufferInfo.offset = 0;
			lightBufferInfo.range = sizeof(LightData) * REON_MAX_LIGHTS;

			VkDescriptorImageInfo directionalShadowBufferInfo{};
			directionalShadowBufferInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			directionalShadowBufferInfo.imageView = m_DirectionalShadowPass.getShadowViews()[i];
			directionalShadowBufferInfo.sampler = m_DirectionalShadowPass.getShadowSampler();

			std::array<VkWriteDescriptorSet, 3> descriptorWrites{};
			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = m_GlobalDescriptorSets[i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &globalBufferInfo;

			descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].dstSet = m_GlobalDescriptorSets[i];
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].dstArrayElement = 0;
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			descriptorWrites[1].descriptorCount = 1;
			descriptorWrites[1].pBufferInfo = &lightBufferInfo;

			descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[2].dstSet = m_GlobalDescriptorSets[i];
			descriptorWrites[2].dstBinding = 2;
			descriptorWrites[2].dstArrayElement = 0;
			descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[2].descriptorCount = 1;
			descriptorWrites[2].pImageInfo = &directionalShadowBufferInfo;

			vkUpdateDescriptorSets(m_Context->getDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}
	}

	void RenderManager::createGlobalBuffers()
	{
		VkDeviceSize bufferSize = sizeof(GlobalRenderData);

		m_GlobalDataBuffers.resize(m_Context->getAmountOfSwapChainImages());
		m_GlobalDataBufferAllocations.resize(m_Context->getAmountOfSwapChainImages());
		m_GlobalDataBuffersMapped.resize(m_Context->getAmountOfSwapChainImages());

		m_LightDataBuffers.resize(m_Context->getAmountOfSwapChainImages());
		m_LightDataBufferAllocations.resize(m_Context->getAmountOfSwapChainImages());
		m_LightDataBuffersMapped.resize(m_Context->getAmountOfSwapChainImages());

		for (size_t i = 0; i < m_Context->getAmountOfSwapChainImages(); i++) {
			// TODO: use the VMA_ALLOCATION_CREATE_MAPPED_BIT along with VmaAllocationInfo object instead
			m_Context->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, m_GlobalDataBuffers[i], m_GlobalDataBufferAllocations[i]);
			vmaMapMemory(m_Context->getAllocator(), m_GlobalDataBufferAllocations[i], &m_GlobalDataBuffersMapped[i]);

			m_Context->createBuffer(sizeof(LightData) * REON_MAX_LIGHTS, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, m_LightDataBuffers[i], m_LightDataBufferAllocations[i]);
			vmaMapMemory(m_Context->getAllocator(), m_LightDataBufferAllocations[i], &m_LightDataBuffersMapped[i]);
		}
	}

	void RenderManager::createOpaqueMaterialDescriptorSets(Material& material)
	{
		material.descriptorSets.resize(m_Context->MAX_FRAMES_IN_FLIGHT);

		std::vector<VkDescriptorSetLayout> materialLayouts(m_Context->MAX_FRAMES_IN_FLIGHT, m_OpaqueMaterialDescriptorSetLayout);
		VkDescriptorSetAllocateInfo materialAllocInfo{};
		materialAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		materialAllocInfo.descriptorPool = m_Context->getDescriptorPool();
		materialAllocInfo.descriptorSetCount = static_cast<uint32_t>(m_Context->MAX_FRAMES_IN_FLIGHT);
		materialAllocInfo.pSetLayouts = materialLayouts.data();

		VkResult res = vkAllocateDescriptorSets(m_Context->getDevice(), &materialAllocInfo, material.descriptorSets.data());
		REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to allocate descriptor sets");

		for (size_t i = 0; i < m_Context->MAX_FRAMES_IN_FLIGHT; i++) {
			VkDescriptorBufferInfo materialBufferInfo{};
			materialBufferInfo.buffer = material.flatDataBuffers[i];
			materialBufferInfo.offset = 0;
			materialBufferInfo.range = sizeof(FlatData);

			VkDescriptorImageInfo albedoImageInfo{};
			const auto& albedoTexture = material.albedoTexture.Get<Texture>();
			albedoImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			albedoImageInfo.imageView = albedoTexture ? albedoTexture->getTextureView() : m_DummyImageView;
			albedoImageInfo.sampler = albedoTexture ? albedoTexture->getSampler() : m_DummySampler;

			VkDescriptorImageInfo normalImageInfo{};
			const auto& normalTexture = material.normalTexture.Get<Texture>();
			normalImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			normalImageInfo.imageView = normalTexture ? normalTexture->getTextureView() : m_DummyImageView;
			normalImageInfo.sampler = normalTexture ? normalTexture->getSampler() : m_DummySampler;

			VkDescriptorImageInfo roughnessMetallicImageInfo{};
			const auto& roughnessMetallicTexture = material.metallicRoughnessTexture.Get<Texture>();
			roughnessMetallicImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			roughnessMetallicImageInfo.imageView = roughnessMetallicTexture ? roughnessMetallicTexture->getTextureView() : m_DummyImageView;
			roughnessMetallicImageInfo.sampler = roughnessMetallicTexture ? roughnessMetallicTexture->getSampler() : m_DummySampler;

			VkDescriptorImageInfo emissiveImageInfo{};
			const auto& emissiveTexture = material.emissiveTexture.Get<Texture>();
			emissiveImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			emissiveImageInfo.imageView = emissiveTexture ? emissiveTexture->getTextureView() : m_DummyImageView;
			emissiveImageInfo.sampler = emissiveTexture ? emissiveTexture->getSampler() : m_DummySampler;

			VkDescriptorImageInfo specularImageInfo{};
			const auto& specularTexture = material.specularTexture.Get<Texture>();
			specularImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			specularImageInfo.imageView = specularTexture ? specularTexture->getTextureView() : m_DummyImageView;
			specularImageInfo.sampler = specularTexture ? specularTexture->getSampler() : m_DummySampler;

			VkDescriptorImageInfo specularColorImageInfo{};
			const auto& specularColorTexture = material.specularColorTexture.Get<Texture>();
			specularColorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			specularColorImageInfo.imageView = specularColorTexture ? specularColorTexture->getTextureView() : m_DummyImageView;
			specularColorImageInfo.sampler = specularColorTexture ? specularColorTexture->getSampler() : m_DummySampler;

			std::array<VkWriteDescriptorSet, 7> descriptorWrites{};
			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = material.descriptorSets[i];
			descriptorWrites[0].dstBinding = 1;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &materialBufferInfo;

			descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].dstSet = material.descriptorSets[i];
			descriptorWrites[1].dstBinding = 3;
			descriptorWrites[1].dstArrayElement = 0;
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[1].descriptorCount = 1;
			descriptorWrites[1].pImageInfo = &albedoImageInfo;

			descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[2].dstSet = material.descriptorSets[i];
			descriptorWrites[2].dstBinding = 4; //MIGHT BE WRONG
			descriptorWrites[2].dstArrayElement = 0;
			descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[2].descriptorCount = 1;
			descriptorWrites[2].pImageInfo = &normalImageInfo;

			descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[3].dstSet = material.descriptorSets[i];
			descriptorWrites[3].dstBinding = 5; //MIGHT BE WRONG
			descriptorWrites[3].dstArrayElement = 0;
			descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[3].descriptorCount = 1;
			descriptorWrites[3].pImageInfo = &roughnessMetallicImageInfo;

			descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[4].dstSet = material.descriptorSets[i];
			descriptorWrites[4].dstBinding = 6; //MIGHT BE WRONG
			descriptorWrites[4].dstArrayElement = 0;
			descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[4].descriptorCount = 1;
			descriptorWrites[4].pImageInfo = &emissiveImageInfo;

			descriptorWrites[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[5].dstSet = material.descriptorSets[i];
			descriptorWrites[5].dstBinding = 7; //MIGHT BE WRONG
			descriptorWrites[5].dstArrayElement = 0;
			descriptorWrites[5].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[5].descriptorCount = 1;
			descriptorWrites[5].pImageInfo = &specularImageInfo;

			descriptorWrites[6].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[6].dstSet = material.descriptorSets[i];
			descriptorWrites[6].dstBinding = 8; //MIGHT BE WRONG
			descriptorWrites[6].dstArrayElement = 0;
			descriptorWrites[6].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[6].descriptorCount = 1;
			descriptorWrites[6].pImageInfo = &specularColorImageInfo;

			vkUpdateDescriptorSets(m_Context->getDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}
	}

	void RenderManager::createOpaqueObjectDescriptorSets(const std::shared_ptr<Renderer>& renderer)
	{
		renderer->objectDescriptorSets.resize(m_Context->MAX_FRAMES_IN_FLIGHT);

		std::vector<VkDescriptorSetLayout> objectLayouts(m_Context->MAX_FRAMES_IN_FLIGHT, m_OpaqueObjectDescriptorSetLayout);
		VkDescriptorSetAllocateInfo objectAllocInfo{};
		objectAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		objectAllocInfo.descriptorPool = m_Context->getDescriptorPool();
		objectAllocInfo.descriptorSetCount = static_cast<uint32_t>(m_Context->MAX_FRAMES_IN_FLIGHT);
		objectAllocInfo.pSetLayouts = objectLayouts.data();

		VkResult res = vkAllocateDescriptorSets(m_Context->getDevice(), &objectAllocInfo, renderer->objectDescriptorSets.data());
		REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to allocate descriptor sets");

		for (size_t i = 0; i < m_Context->MAX_FRAMES_IN_FLIGHT; i++) {
			VkDescriptorBufferInfo objectBufferInfo{};
			objectBufferInfo.buffer = renderer->objectDataBuffers[i];
			objectBufferInfo.offset = 0;
			objectBufferInfo.range = sizeof(ObjectRenderData);

			std::array<VkWriteDescriptorSet, 1> descriptorWrites{};
			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = renderer->objectDescriptorSets[i];
			descriptorWrites[0].dstBinding = 2;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &objectBufferInfo;

			vkUpdateDescriptorSets(m_Context->getDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}

		m_DirectionalShadowPass.createPerObjectDescriptorSets(m_Context, renderer);
	}

	void RenderManager::createOpaqueGraphicsPipelines()
	{
		uint32_t mostUsedFlags = AlbedoTexture | NormalTexture | MetallicRoughnessTexture;

		std::vector<uint32_t> prebakePermutations{
			0,
			AlbedoTexture | NormalTexture | MetallicRoughnessTexture | OcclusionTexture,
			AlbedoTexture | NormalTexture | MetallicRoughnessTexture | OcclusionTexture | EmissiveTexture,
			NormalTexture | MetallicRoughnessTexture,
			MetallicRoughnessTexture,
			AlbedoTexture | NormalTexture,
		};

		auto vertShaderCode = Shader::CompileHLSLToSPIRV("Assets/Shaders/vert.vert", mostUsedFlags);
		auto fragShaderCode = Shader::CompileHLSLToSPIRV("Assets/Shaders/frag.frag", mostUsedFlags);

		if (vertShaderCode.empty() || fragShaderCode.empty()) {
			REON_CORE_ERROR("Shader code not compiled correctly");
			return;
		}

		VkShaderModule vertShaderModule = m_Context->createShaderModule(vertShaderCode);
		VkShaderModule fragShaderModule = m_Context->createShaderModule(fragShaderCode);

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
		multisampling.rasterizationSamples = m_Context->getSampleCount();
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
		};

		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();

		std::array<VkDescriptorSetLayout, 3> setLayouts{ m_OpaqueGlobalDescriptorSetLayout, m_OpaqueMaterialDescriptorSetLayout, m_OpaqueObjectDescriptorSetLayout };
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = setLayouts.size();
		pipelineLayoutInfo.pSetLayouts = setLayouts.data();
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.pPushConstantRanges = nullptr;

		VkResult res = vkCreatePipelineLayout(m_Context->getDevice(), &pipelineLayoutInfo, nullptr, &m_OpaquePipelineLayout);
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
		pipelineInfo.layout = m_OpaquePipelineLayout;
		pipelineInfo.renderPass = m_OpaqueRenderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex = -1;
		pipelineInfo.flags = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;

		res = vkCreateGraphicsPipelines(m_Context->getDevice(), m_PipelineCache, 1, &pipelineInfo, nullptr, &m_OpaqueGraphicsPipeline);
		REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to create graphics pipeline")

		vkDestroyShaderModule(m_Context->getDevice(), fragShaderModule, nullptr);
		vkDestroyShaderModule(m_Context->getDevice(), vertShaderModule, nullptr);

		m_PipelineByMaterialPermutation.insert({ mostUsedFlags, m_OpaqueGraphicsPipeline });

		for (auto permutation : prebakePermutations) {
			m_PipelineByMaterialPermutation.insert({ permutation, createGraphicsPipeline(m_OpaqueGraphicsPipeline, permutation)});
		}
	}

	void RenderManager::createPipelineCache()
	{
		VkPipelineCacheCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
		VkResult res = vkCreatePipelineCache(m_Context->getDevice(), &createInfo, nullptr, &m_PipelineCache);
		REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to create pipeline cache");
	}

	VkPipeline RenderManager::getPipelineFromFlags(uint32_t flags)
	{
		if (m_PipelineByMaterialPermutation.contains(flags)) {
			return m_PipelineByMaterialPermutation[flags];
		}
		else {
			VkPipeline newPipeline = createGraphicsPipeline(m_OpaqueGraphicsPipeline, flags);
			m_PipelineByMaterialPermutation[flags] = newPipeline;
			return newPipeline;
		}
	}

	VkPipeline RenderManager::createGraphicsPipeline(VkPipeline basePipeline, uint32_t flags)
	{
		REON_CORE_INFO("Creating pipeline with flags: {}", flags);

		auto vertShaderCode = Shader::CompileHLSLToSPIRV("Assets/Shaders/vert.vert", flags);
		auto fragShaderCode = Shader::CompileHLSLToSPIRV("Assets/Shaders/frag.frag", flags);

		if (vertShaderCode.empty() || fragShaderCode.empty()) {
			REON_CORE_ERROR("Shader code not compiled correctly");
			return {};
		}

		VkShaderModule vertShaderModule = m_Context->createShaderModule(vertShaderCode);
		VkShaderModule fragShaderModule = m_Context->createShaderModule(fragShaderCode);

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
		multisampling.rasterizationSamples = m_Context->getSampleCount();
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
		};

		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();

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
		pipelineInfo.layout = m_OpaquePipelineLayout;
		pipelineInfo.renderPass = m_OpaqueRenderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.flags = VK_PIPELINE_CREATE_DERIVATIVE_BIT;
		pipelineInfo.basePipelineIndex = -1;
		pipelineInfo.basePipelineHandle = m_OpaqueGraphicsPipeline;

		VkPipeline pipeline;

		VkResult res = vkCreateGraphicsPipelines(m_Context->getDevice(), m_PipelineCache, 1, &pipelineInfo, nullptr, &pipeline); //TODO: optimise this to only do 1 call to createpipelines instead of one for every permutation

		vkDestroyShaderModule(m_Context->getDevice(), fragShaderModule, nullptr);
		vkDestroyShaderModule(m_Context->getDevice(), vertShaderModule, nullptr);

		return pipeline;
	}

	void RenderManager::HotReloadShaders() {
		vkDeviceWaitIdle(m_Context->getDevice());

		vkDestroyPipelineLayout(m_Context->getDevice(), m_OpaquePipelineLayout, nullptr);
		for (auto [permutation, pipeline] : m_PipelineByMaterialPermutation) {
			vkDestroyPipeline(m_Context->getDevice(), pipeline, nullptr);
		}

		m_PipelineByMaterialPermutation.clear();

		createOpaqueGraphicsPipelines();

		m_TransparentPass.hotReloadShaders(m_Context);
	}

	VkDescriptorSet RenderManager::GetEndBuffer()
	{
		if (resized || m_DrawCommandsByShaderMaterial.empty()) {
			return nullptr;
		}
		return m_EndDescriptorSets[m_Context->getCurrentImageIndex()];
	}

	void RenderManager::cleanup()
	{
		//vkDeviceWaitIdle(m_Context->getDevice());

		deleteForResize();

		for (auto& renderer : m_Renderers) {
			renderer->cleanup();
		}

		m_DirectionalShadowPass.cleanup(m_Context);

		//vkFreeDescriptorSets(m_Context->getDevice(), m_Context->getDescriptorPool(), m_EndDescriptorSets.size(), m_EndDescriptorSets.data());

		vkDestroyImage(m_Context->getDevice(), m_DummyImage, nullptr);
		vmaFreeMemory(m_Context->getAllocator(), m_DummyImageAllocation);
		vkDestroyImageView(m_Context->getDevice(), m_DummyImageView, nullptr);
		vkDestroySampler(m_Context->getDevice(), m_DummySampler, nullptr);

		//vkFreeDescriptorSets(m_Context->getDevice(), m_Context->getDescriptorPool(), m_GlobalDescriptorSets.size(), m_GlobalDescriptorSets.data());

		for (int i = 0; i < m_Context->getAmountOfSwapChainImages(); i++) {
			vkDestroyBuffer(m_Context->getDevice(), m_GlobalDataBuffers[i], nullptr);
			vmaUnmapMemory(m_Context->getAllocator(), m_GlobalDataBufferAllocations[i]);
			vmaFreeMemory(m_Context->getAllocator(), m_GlobalDataBufferAllocations[i]);

			vkDestroyBuffer(m_Context->getDevice(), m_LightDataBuffers[i], nullptr);
			vmaUnmapMemory(m_Context->getAllocator(), m_LightDataBufferAllocations[i]);
			vmaFreeMemory(m_Context->getAllocator(), m_LightDataBufferAllocations[i]);
		}

		vkDestroySampler(m_Context->getDevice(), m_EndSampler, nullptr);

		vkDestroyDescriptorSetLayout(m_Context->getDevice(), m_EndDescriptorSetLayout, nullptr);
		vkDestroyDescriptorSetLayout(m_Context->getDevice(), m_OpaqueGlobalDescriptorSetLayout, nullptr);
		vkDestroyDescriptorSetLayout(m_Context->getDevice(), m_OpaqueMaterialDescriptorSetLayout, nullptr);
		vkDestroyDescriptorSetLayout(m_Context->getDevice(), m_OpaqueObjectDescriptorSetLayout, nullptr);

		vkDestroyPipeline(m_Context->getDevice(), m_OpaqueGraphicsPipeline, nullptr);
		vkDestroyPipelineLayout(m_Context->getDevice(), m_OpaquePipelineLayout, nullptr);

		vkDestroyRenderPass(m_Context->getDevice(), m_OpaqueRenderPass, nullptr);

		vkDestroyCommandPool(m_Context->getDevice(), m_OpaqueCommandPool, nullptr);
	}

	void RenderManager::setMainLight(std::shared_ptr<Light> light)
	{
		m_DirectionalShadowPass.createPerLightDescriptorSets(m_Context);
	}

	void RenderManager::deleteForResize()
	{
		vkDestroyImage(m_Context->getDevice(), m_ColorImage, nullptr);
		vmaFreeMemory(m_Context->getAllocator(), m_ColorImageAllocation);
		vkDestroyImageView(m_Context->getDevice(), m_ColorImageView, nullptr);

		vkDestroyImage(m_Context->getDevice(), m_DepthImage, nullptr);
		vmaFreeMemory(m_Context->getAllocator(), m_DepthImageAllocation);
		vkDestroyImageView(m_Context->getDevice(), m_DepthImageView, nullptr);

		for (size_t i = 0; i < m_Context->getAmountOfSwapChainImages(); i++) {
			vkDestroyImage(m_Context->getDevice(), m_OpaqueResolveImages[i], nullptr);
			vmaFreeMemory(m_Context->getAllocator(), m_OpaqueResolveImageAllocations[i]);
			vkDestroyImageView(m_Context->getDevice(), m_OpaqueResolveImageViews[i], nullptr);

			vkDestroyImage(m_Context->getDevice(), m_EndImages[i], nullptr);
			vmaFreeMemory(m_Context->getAllocator(), m_EndImageAllocations[i]);
			vkDestroyImageView(m_Context->getDevice(), m_EndImageViews[i], nullptr);

			vkDestroyImage(m_Context->getDevice(), m_DepthResolveImages[i], nullptr);
			vmaFreeMemory(m_Context->getAllocator(), m_DepthResolveAllocations[i]);
			vkDestroyImageView(m_Context->getDevice(), m_DepthResolveViews[i], nullptr);
		}

		for (size_t i = 0; i < m_Context->getAmountOfSwapChainImages(); i++) {
			vkDestroyFramebuffer(m_Context->getDevice(), m_OpaqueFramebuffers[i], nullptr);
		}

	}

	void RenderManager::SetRenderDimensions(int width, int height)
	{
		resized = true;

		m_Width = width;
		m_Height = height;

		vkDeviceWaitIdle(m_Context->getDevice());

		deleteForResize();

		createOpaqueImages();
		createOpaqueFrameBuffers();
		for (size_t i = 0; i < m_Context->getAmountOfSwapChainImages(); i++) {
			VkDescriptorImageInfo endImageInfo{};
			endImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			endImageInfo.imageView = m_EndImageViews[i];
			endImageInfo.sampler = m_EndSampler;

			std::array<VkWriteDescriptorSet, 1> descriptorWrites{};
			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = m_EndDescriptorSets[i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pImageInfo = &endImageInfo;

			vkUpdateDescriptorSets(m_Context->getDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}

		m_TransparentPass.resize(m_Context, width, height, m_EndImageViews, m_OpaqueResolveImageViews, m_DepthResolveViews);
		m_UnlitPass.resize(m_Context, width, height, m_EndImageViews);
	}

	int RenderManager::GetRenderWidth()
	{
		return m_Width;
	}

	int RenderManager::GetRenderHeight()
	{
		return m_Height;
	}

}