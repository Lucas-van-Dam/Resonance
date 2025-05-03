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
		GenerateShadows();
		glBindFramebuffer(GL_FRAMEBUFFER, m_SceneFbo);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		RenderOpaques();
		RenderSkyBox();
		RenderTransparents();
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		RenderPostProcessing();
	}

	void RenderManager::AddRenderer(const std::shared_ptr<Renderer>& renderer) {
		//m_ShaderToRenderer[renderer->material.Get<Material>()->shader].push_back(renderer);
		m_Renderers.push_back(renderer);
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
		glDepthFunc(GL_LEQUAL);
		glDepthMask(GL_FALSE);
		m_SkyboxShader->use();
		m_SkyboxShader->setMat4("view", glm::mat4(glm::mat3(m_Camera->GetViewMatrix())));
		m_SkyboxShader->setMat4("projection", m_Camera->GetProjectionMatrix());

		glBindVertexArray(m_SkyboxVAO);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_SkyboxTexture);
		m_SkyboxShader->setInt("skybox", 0);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glBindVertexArray(0);
		glDepthMask(GL_TRUE);
		glDepthFunc(GL_LESS);
	}

	void RenderManager::RenderOpaques() {
		REON_CORE_ASSERT(glIsEnabled(GL_DEPTH_TEST), "Depth testing disabled unexpectedly");
		GLboolean depthWriteEnabled;
		glGetBooleanv(GL_DEPTH_WRITEMASK, &depthWriteEnabled);
		REON_CORE_ASSERT(depthWriteEnabled == GL_TRUE, "Depth writing disabled unexpectedly");
		//for (const auto& pair : shaderToRenderer) {
		//    std::shared_ptr<Shader> shader = pair.first;
		//    const auto& renderersFromShader = pair.second;

		//    shader->use();

		//    for (const auto& renderer : renderersFromShader) {
		//        renderer->Draw(mainLightView, mainLightProj, skyboxTexture, irradianceMap, prefilterMap, brdfLUTTexture, depthCubeMaps, depthMap);
		//    }
		//}

		for (const auto& renderer : m_Renderers) {
			renderer->material.Get<Material>()->shader->use();
			renderer->Draw(m_MainLightView, m_MainLightProj, m_SkyboxTexture, m_IrradianceMap, m_PrefilterMap, m_BrdfLUTTexture, m_DepthCubeMaps, m_DepthMap);
		}
	}

	void RenderManager::RenderTransparents() {

	}

	void RenderManager::RenderPostProcessing() {
		m_RenderResultTexture = m_PostProcessingStack.Render(m_SceneTexture, m_SceneDepthTex);
	}


	void RenderManager::GenerateShadows() {
		GenerateMainLightShadows();
		GenerateAdditionalShadows();
	}

	RenderManager::RenderManager(std::shared_ptr<LightManager> lightManager, std::shared_ptr<EditorCamera> camera) : m_LightManager(std::move(lightManager)) {
		m_Width = Application::Get().GetWindow().GetWidth();
		m_Height = Application::Get().GetWindow().GetHeight();

		m_BloomEffect = std::make_shared<BloomEffect>();
		m_PostProcessingStack.AddEffect(m_BloomEffect);
		m_ColorCorrection = std::make_shared<ColorCorrection>();
		m_PostProcessingStack.AddEffect(m_ColorCorrection);
		m_DepthOfField = std::make_shared<DepthOfField>();
		m_PostProcessingStack.AddEffect(m_DepthOfField);
		m_PostProcessingStack.Init(m_Width, m_Height);

		m_KeyPressedCallbackID = EventBus::Get().subscribe<KeyPressedEvent>(REON_BIND_EVENT_FN(RenderManager::OnKeyPressed));

		glEnable(GL_DEPTH_TEST);

		//Initialize main light shadow maps
		this->m_Camera = std::move(camera);
		glGenFramebuffers(1, &m_DepthMapFBO);
		glGenTextures(1, &m_DepthMap);
		glBindTexture(GL_TEXTURE_2D, m_DepthMap);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
			MAIN_SHADOW_WIDTH, MAIN_SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glBindFramebuffer(GL_FRAMEBUFFER, m_DepthMapFBO);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_DepthMap, 0);
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		//Initialize full screen shader
		InitializeFboAndTexture(m_SceneFbo, m_SceneTexture, m_Width, m_Height);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glBindFramebuffer(GL_FRAMEBUFFER, m_SceneFbo);

		glGenTextures(1, &m_SceneDepthTex);
		glBindTexture(GL_TEXTURE_2D, m_SceneDepthTex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, m_Width, m_Height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_SceneDepthTex, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		InitializeFboAndTexture(m_PostProcessFbo, m_PostProcessTexture, m_Width, m_Height);


		glGenVertexArrays(1, &QuadVAO);
		glGenBuffers(1, &m_QuadVBO);
		glBindVertexArray(QuadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, m_QuadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

		//Initialize skybox
		InitializeSkyBox();
	}

	void RenderManager::InitializeFboAndTexture(uint& fbo, uint& texture, int width, int height) {
		glGenFramebuffers(1, &fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);

		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

		REON_CORE_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "framebuffer is not complete!");

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void RenderManager::RenderFullScreenQuad()
	{
		glBindVertexArray(QuadVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);
	}

	void RenderManager::OnKeyPressed(const KeyPressedEvent& event)
	{
		if (event.GetKeyCode() == REON_KEY_I && event.GetRepeatCount() == 0) {
			m_PostProcessingStack.StartProfiling();
		}
		if (event.GetKeyCode() == REON_KEY_O && event.GetRepeatCount() == 0) {
			m_PostProcessingStack.ExportFrameDataToCSV("BloomFrameData.csv", "Bloom");
		}
		if (event.GetKeyCode() == REON_KEY_K && event.GetRepeatCount() == 0) {
			m_PostProcessingStack.ExportFrameDataToCSV("DepthOfFieldFrameData.csv", "Depth of Field");
		}
	}

	void RenderManager::GenerateMainLightShadows() {
		std::shared_ptr<Light> light = m_LightManager->mainLight;
		if (light == nullptr) {
			printf("No main light found\n");
			return;
		}
		glm::mat4 lightSpaceMatrix;
		float near_plane = -50.0f, far_plane = 100;
		m_MainLightProj = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, near_plane, far_plane);

		m_MainLightView = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), (light->GetOwner()->GetTransform()->localRotation * glm::vec3(0.0f, 0.0f, 1.0f)), (light->GetOwner()->GetTransform()->localRotation * glm::vec3(0.0f, 1.0f, 0.0f)));
		lightSpaceMatrix = m_MainLightProj * m_MainLightView;



		glViewport(0, 0, MAIN_SHADOW_WIDTH, MAIN_SHADOW_HEIGHT);
		glBindFramebuffer(GL_FRAMEBUFFER, m_DepthMapFBO);
		glClear(GL_DEPTH_BUFFER_BIT);
		m_DirectionalShadowShader->use();
		m_DirectionalShadowShader->setMat4("lightSpaceMatrix", lightSpaceMatrix);
		glDisable(GL_CULL_FACE);
		glDepthFunc(GL_LESS);
		for (const auto& renderer : m_Renderers) {
			renderer->Draw(m_MainLightView, m_MainLightProj, -1, -1, -1, -1, std::vector<int>(), 0, m_DirectionalShadowShader);
		}
		glEnable(GL_CULL_FACE);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glViewport(0, 0, m_Width, m_Height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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
				m_AdditionalShadowShader->setMat4("shadowMatrices[" + std::to_string(j) + "]", shadowTransforms[j]);
			m_AdditionalShadowShader->setFloat("far_plane", far_plane);
			m_AdditionalShadowShader->setVec3("lightPos", lightPos);
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
		for (const std::shared_ptr<Light>& light : m_LightManager->lights) {
			if (light->type == LightType::Point) {
				m_PointLights.emplace_back(light);
				unsigned int depthCubemap, depthFBO;
				glGenFramebuffers(1, &depthFBO);
				glGenTextures(1, &depthCubemap);
				glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
				for (unsigned int i = 0; i < 6; ++i)
					glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, ADDITIONAL_SHADOW_WIDTH, ADDITIONAL_SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
				// attach depth texture as FBO's depth buffer
				glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);
				glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemap, 0);
				glDrawBuffer(GL_NONE);
				glReadBuffer(GL_NONE);
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				m_AdditionalDepthFBOs.emplace_back(depthFBO);
				m_DepthCubeMaps.emplace_back(depthCubemap);
			}
		}
	}

	void RenderManager::InitializeSkyBox()
	{
		//setup skybox vertex data
		glGenVertexArrays(1, &m_SkyboxVAO);
		glGenBuffers(1, &m_SkyboxVBO);
		glBindVertexArray(m_SkyboxVAO);
		glBindBuffer(GL_ARRAY_BUFFER, m_SkyboxVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
		//setup framebuffer
		unsigned int captureFBO;
		unsigned int captureRBO;
		glGenFramebuffers(1, &captureFBO);
		glGenRenderbuffers(1, &captureRBO);

		glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
		glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);
		//load hdr texture
		stbi_set_flip_vertically_on_load(true);
		int width, height, nrComponents;
		float* data = stbi_loadf(m_SkyboxLocation.c_str(), &width, &height, &nrComponents, 0);
		if (data)
		{
			glGenTextures(1, &m_HdrTexture);
			glBindTexture(GL_TEXTURE_2D, m_HdrTexture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			stbi_image_free(data);
		}
		else
		{
			std::cout << "Failed to load HDR image." << std::endl;
		}
		//setup cubemap for capturing
		glGenTextures(1, &m_SkyboxTexture);
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_SkyboxTexture);
		for (unsigned int i = 0; i < 6; ++i)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 512, 512, 0, GL_RGB, GL_FLOAT, nullptr);
		}
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		//setup projection and view matrices
		glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
		glm::mat4 captureViews[] =
		{
				glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
				glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
				glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
				glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
				glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
				glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
		};
		//map hdr environment to cubemap
		m_SkyboxMappingShader->use();
		m_SkyboxMappingShader->setInt("equirectangularMap", 0);
		m_SkyboxMappingShader->setMat4("projection", captureProjection);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_HdrTexture);

		glViewport(0, 0, 512, 512); // don't forget to configure the viewport to the capture dimensions.
		glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
		for (unsigned int i = 0; i < 6; ++i)
		{
			m_SkyboxMappingShader->setMat4("view", captureViews[i]);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_SkyboxTexture, 0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glBindVertexArray(m_SkyboxVAO);
			glDrawArrays(GL_TRIANGLES, 0, 36);
			glBindVertexArray(0);
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glBindTexture(GL_TEXTURE_CUBE_MAP, m_SkyboxTexture);
		glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
		//setup irradiance map
		glGenTextures(1, &m_IrradianceMap);
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_IrradianceMap);
		for (unsigned int i = 0; i < 6; ++i)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 32, 32, 0, GL_RGB, GL_FLOAT, nullptr);
		}
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
		glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);
		//create the irradiance map
		m_IrradianceShader->use();
		m_IrradianceShader->setInt("environmentMap", 0);
		m_IrradianceShader->setMat4("projection", captureProjection);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_SkyboxTexture);

		glViewport(0, 0, 32, 32); // don't forget to configure the viewport to the capture dimensions.
		glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
		for (unsigned int i = 0; i < 6; ++i)
		{
			m_IrradianceShader->setMat4("view", captureViews[i]);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_IrradianceMap, 0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glBindVertexArray(m_SkyboxVAO);
			glDrawArrays(GL_TRIANGLES, 0, 36);
			glBindVertexArray(0);
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		// setup pre-filter cubemap
		glGenTextures(1, &m_PrefilterMap);
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_PrefilterMap);
		for (unsigned int i = 0; i < 6; ++i)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 128, 128, 0, GL_RGB, GL_FLOAT, nullptr);
		}

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		// generate mipmaps for the cubemap so OpenGL automatically allocates the required memory.
		glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

		m_PreFilterShader->use();
		m_PreFilterShader->setInt("environmentMap", 0);
		m_PreFilterShader->setMat4("projection", captureProjection);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_SkyboxTexture);

		glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
		unsigned int maxMipLevels = 5;
		for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
		{
			// reisze framebuffer according to mip-level size.
			unsigned int mipWidth = static_cast<unsigned int>(128 * std::pow(0.5, mip));
			unsigned int mipHeight = static_cast<unsigned int>(128 * std::pow(0.5, mip));
			glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
			glViewport(0, 0, mipWidth, mipHeight);

			float roughness = (float)mip / (float)(maxMipLevels - 1);
			m_PreFilterShader->setFloat("roughness", roughness);
			for (unsigned int i = 0; i < 6; ++i)
			{
				m_PreFilterShader->setMat4("view", captureViews[i]);
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_PrefilterMap, mip);

				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				glBindVertexArray(m_SkyboxVAO);
				glDrawArrays(GL_TRIANGLES, 0, 36);
				glBindVertexArray(0);
			}
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glGenTextures(1, &m_BrdfLUTTexture);

		glBindTexture(GL_TEXTURE_2D, m_BrdfLUTTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// then re-configure capture framebuffer object and render screen-space quad with BRDF shader.
		glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
		glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_BrdfLUTTexture, 0);

		glViewport(0, 0, 512, 512);
		m_BrdfShader->use();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glBindVertexArray(QuadVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glViewport(0, 0, width, height);
	}

	void RenderManager::HotReloadShaders() {
		for (std::shared_ptr<Renderer> renderer : m_Renderers) {
			renderer->material.Get<Material>()->shader->ReloadShader(); //TODO: reload every shader once, instead of every material to save performance
		}
		m_DirectionalShadowShader->ReloadShader();
		m_AdditionalShadowShader->ReloadShader();
		m_PostProcessingStack.HotReloadShaders();
	}

	unsigned int RenderManager::GetEndBuffer()
	{
		return m_RenderResultTexture;
	}

	void RenderManager::SetRenderDimensions(int width, int height)
	{
		m_Width = width;
		m_Height = height;
		glBindFramebuffer(GL_FRAMEBUFFER, m_SceneFbo);
		glBindTexture(GL_TEXTURE_2D, m_SceneTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_SceneTexture, 0);

		glBindTexture(GL_TEXTURE_2D, m_SceneDepthTex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_SceneDepthTex, 0);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			std::cerr << "Framebuffer incomplete after resizing depth texture!" << std::endl;
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		m_PostProcessingStack.Resize(width, height);
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