#include "reonpch.h"
#include "RenderManager.h"

#include "REON/GameHierarchy/Components/Transform.h"
#include "REON/GameHierarchy/GameObject.h"
#include "stb_image_wrapper.h"
#include "REON/Application.h"
#include "REON/EditorCamera.h"

namespace REON {

	void RenderManager::Render() {
		GenerateShadows();
		RenderOpaques();
		RenderTransparents();
		RenderSkyBox();
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
		m_SkyboxShader->use();
		m_SkyboxShader->setMat4("view", glm::mat4(glm::mat3(m_Camera->GetViewMatrix())));
		m_SkyboxShader->setMat4("projection", m_Camera->GetProjectionMatrix());

		glBindVertexArray(m_SkyboxVAO);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_SkyboxTexture);
		m_SkyboxShader->setInt("skybox", 0);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glBindVertexArray(0);
		glDepthFunc(GL_LESS);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void RenderManager::RenderOpaques() {
		glBindFramebuffer(GL_FRAMEBUFFER, m_SceneFramebuffer);
		glEnable(GL_DEPTH_TEST);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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
		glBindFramebuffer(GL_FRAMEBUFFER, m_PostProcessFbo);
		glDisable(GL_DEPTH_TEST);
		glClear(GL_COLOR_BUFFER_BIT);
		m_ScreenShader->use();
		glBindVertexArray(m_QuadVAO);
		glActiveTexture(GL_TEXTURE0);
		glUniform1i(glGetUniformLocation(m_ScreenShader->ID, "screenTexture"), 0);
		glBindTexture(GL_TEXTURE_2D, m_SceneTexture);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glEnable(GL_DEPTH_TEST);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}


	void RenderManager::GenerateShadows() {
		GenerateMainLightShadows();
		GenerateAdditionalShadows();
	}

	RenderManager::RenderManager(std::shared_ptr<LightManager> lightManager, std::shared_ptr<EditorCamera> camera) : m_LightManager(std::move(lightManager)) {
		m_Width = Application::Get().GetWindow().GetWidth();
		m_Height = Application::Get().GetWindow().GetHeight();

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
		glGenFramebuffers(1, &m_SceneFramebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, m_SceneFramebuffer);

		glGenTextures(1, &m_SceneTexture);
		glBindTexture(GL_TEXTURE_2D, m_SceneTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, m_Width, m_Height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_SceneTexture, 0);

		REON_CORE_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Scene framebuffer is not complete!");

		glGenRenderbuffers(1, &m_Rbo);
		glBindRenderbuffer(GL_RENDERBUFFER, m_Rbo);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_Width, m_Height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_Rbo);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glGenFramebuffers(1, &m_PostProcessFbo);
		glBindFramebuffer(GL_FRAMEBUFFER, m_PostProcessFbo);

		glGenTextures(1, &m_PostProcessTexture);
		glBindTexture(GL_TEXTURE_2D, m_PostProcessTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, m_Width, m_Height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_PostProcessTexture, 0);

		REON_CORE_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Post-process framebuffer is not complete!");

		glGenVertexArrays(1, &m_QuadVAO);
		glGenBuffers(1, &m_QuadVBO);
		glBindVertexArray(m_QuadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, m_QuadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

		//Initialize skybox
		InitializeSkyBox();
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
		glBindVertexArray(m_QuadVAO);
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
		m_ScreenShader->ReloadShader();
	}

	unsigned int RenderManager::GetEndBuffer()
	{
		return m_PostProcessTexture;
	}

	void RenderManager::SetRenderDimensions(int width, int height)
	{
		m_Width = width;
		m_Height = height;
		glBindTexture(GL_TEXTURE_2D, m_SceneTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, m_Width, m_Height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
		glBindRenderbuffer(GL_RENDERBUFFER, m_Rbo);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_Width, m_Height);

		glBindTexture(GL_TEXTURE_2D, m_PostProcessTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, m_Width, m_Height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
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