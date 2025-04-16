#pragma once

#include "REON/GameHierarchy/Components/Renderer.h"
#include "REON/Rendering/LightManager.h"
#include "REON/ResourceManagement/ResourceManager.h"
#include "PostProcessing/PostProcessingStack.h"
#include "PostProcessing/BloomEffect.h"


namespace REON {
	class GameObject;
	class EditorCamera;

	class RenderManager {
	public:
		void Render();
		void AddRenderer(const std::shared_ptr<Renderer>& renderer);
		void RemoveRenderer(std::shared_ptr<Renderer> renderer);
		RenderManager(std::shared_ptr<LightManager> lightManager, std::shared_ptr<EditorCamera> camera);
		void Initialize();
		void HotReloadShaders();
		uint GetEndBuffer();
		void SetRenderDimensions(int width, int height);
		int GetRenderWidth();
		int GetRenderHeight();
		static void InitializeFboAndTexture(uint& fbo, uint& texture, int width, int height);

	private:
		void RenderOpaques();
		void RenderTransparents();
		void RenderPostProcessing();
		void GenerateShadows();
		void GenerateMainLightShadows();
		void GenerateAdditionalShadows();
		void RenderSkyBox();
		void InitializeSkyBox();

	private:
		int m_Width, m_Height;
		std::unordered_map<std::shared_ptr<Shader>, std::vector<std::shared_ptr<Renderer>>> m_ShaderToRenderer;
		std::vector<std::shared_ptr<Renderer>> m_Renderers;
		std::shared_ptr<EditorCamera> m_Camera;

		//Lighting
		std::shared_ptr<LightManager> m_LightManager;
		glm::mat4 m_MainLightView;
		glm::mat4 m_MainLightProj;
		std::vector<std::shared_ptr<Light>> m_PointLights;

		//Shadows
		uint m_DepthMapFBO;
		uint m_DepthMap;
		const uint MAIN_SHADOW_WIDTH = 4096, MAIN_SHADOW_HEIGHT = 4096;
		const uint ADDITIONAL_SHADOW_WIDTH = 1024, ADDITIONAL_SHADOW_HEIGHT = 1024;
		std::vector<int> m_DepthCubeMaps;
		std::vector<uint> m_AdditionalDepthFBOs;
		std::shared_ptr<Shader> m_DirectionalShadowShader = ResourceManager::GetInstance().LoadResource<Shader>("DirectionalShadowShader", std::make_tuple("DirectionalShadow.vert", "DirectionalShadow.frag", std::optional<std::string>{}));
		std::shared_ptr<Shader> m_AdditionalShadowShader = ResourceManager::GetInstance().LoadResource<Shader>("OmnidirectionalShadowShader", std::make_tuple("OmnidirectionalShadow.vert", "OmnidirectionalShadow.frag", std::optional<std::string>{"OmnidirectionalShadow.geom"}));

		//Skybox
		uint m_SkyboxVAO, m_SkyboxVBO;
		uint m_SkyboxTexture;
		uint m_HdrTexture;
		uint m_IrradianceMap;
		uint m_PrefilterMap;
		uint m_BrdfLUTTexture;
		std::shared_ptr<Shader> m_IrradianceShader = ResourceManager::GetInstance().LoadResource<Shader>("IrradianceShader", std::make_tuple("CubeProjection.vert", "IrradianceMap.frag", std::optional<std::string>{}));
		std::shared_ptr<Shader> m_SkyboxShader = ResourceManager::GetInstance().LoadResource<Shader>("SkyboxShader", std::make_tuple("SkyBox.vert", "SkyBox.frag", std::optional<std::string>{}));
		std::shared_ptr<Shader> m_SkyboxMappingShader = ResourceManager::GetInstance().LoadResource<Shader>("SkyboxMappingShader", std::make_tuple("CubeProjection.vert", "CubeProjection.frag", std::optional<std::string>{}));
		std::shared_ptr<Shader> m_PreFilterShader = ResourceManager::GetInstance().LoadResource<Shader>("PreFilterShader", std::make_tuple("CubeProjection.vert", "PreFilter.frag", std::optional<std::string>{}));
		std::shared_ptr<Shader> m_BrdfShader = ResourceManager::GetInstance().LoadResource<Shader>("BrdfShader", std::make_tuple("brdf.vert", "brdf.frag", std::optional<std::string>{}));
		std::string m_SkyboxLocation = "Assets/Textures/brown_photostudio_02_4k.hdr";

		//Screen shader
		uint m_SceneFbo, m_SceneTexture, m_SceneDepthTex;
	public:
		static uint m_QuadVAO;

	private:
		uint m_QuadVBO;
		float quadVertices[24] = { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
			// positions   // texCoords
			-1.0f,  1.0f,  0.0f, 1.0f,
			-1.0f, -1.0f,  0.0f, 0.0f,
			1.0f, -1.0f,  1.0f, 0.0f,

			-1.0f,  1.0f,  0.0f, 1.0f,
			1.0f, -1.0f,  1.0f, 0.0f,
			1.0f,  1.0f,  1.0f, 1.0f
		};

		//Post-processing
		PostProcessingStack m_PostProcessingStack;
		uint m_PostProcessTexture, m_PostProcessFbo;
		uint m_RenderResultTexture;
		//Bloom
		uint m_BloomFbo, m_BloomTexture;
		uint m_BlurFboHorizontal, m_BlurTextureHorizontal;
		uint m_BlurFboVertical, m_BlurTextureVertical;
		std::shared_ptr<Shader> m_BrightPassShader = ResourceManager::GetInstance().LoadResource<Shader>("BrightPassShader", std::make_tuple("fullScreen.vert", "BrightPassShader.frag", std::optional<std::string>{}));
		std::shared_ptr<Shader> m_BlurPassShader = ResourceManager::GetInstance().LoadResource<Shader>("BlurPassShader", std::make_tuple("fullScreen.vert", "BlurShader.frag", std::optional<std::string>{}));
		std::shared_ptr<Shader> m_CompositeShader = ResourceManager::GetInstance().LoadResource<Shader>("CompositeShader", std::make_tuple("fullScreen.vert", "BloomComposite.frag", std::optional<std::string>{}));

	public:
		static std::shared_ptr<BloomEffect> m_BloomEffect;
	private:

#pragma region data
		float skyboxVertices[108] = {
			// positions
			-1.0f,  1.0f, -1.0f,
			-1.0f, -1.0f, -1.0f,
			1.0f, -1.0f, -1.0f,
			1.0f, -1.0f, -1.0f,
			1.0f,  1.0f, -1.0f,
			-1.0f,  1.0f, -1.0f,

			-1.0f, -1.0f,  1.0f,
			-1.0f, -1.0f, -1.0f,
			-1.0f,  1.0f, -1.0f,
			-1.0f,  1.0f, -1.0f,
			-1.0f,  1.0f,  1.0f,
			-1.0f, -1.0f,  1.0f,

			1.0f, -1.0f, -1.0f,
			1.0f, -1.0f,  1.0f,
			1.0f,  1.0f,  1.0f,
			1.0f,  1.0f,  1.0f,
			1.0f,  1.0f, -1.0f,
			1.0f, -1.0f, -1.0f,

			-1.0f, -1.0f,  1.0f,
			-1.0f,  1.0f,  1.0f,
			1.0f,  1.0f,  1.0f,
			1.0f,  1.0f,  1.0f,
			1.0f, -1.0f,  1.0f,
			-1.0f, -1.0f,  1.0f,

			-1.0f,  1.0f, -1.0f,
			1.0f,  1.0f, -1.0f,
			1.0f,  1.0f,  1.0f,
			1.0f,  1.0f,  1.0f,
			-1.0f,  1.0f,  1.0f,
			-1.0f,  1.0f, -1.0f,

			-1.0f, -1.0f, -1.0f,
			-1.0f, -1.0f,  1.0f,
			1.0f, -1.0f, -1.0f,
			1.0f, -1.0f, -1.0f,
			-1.0f, -1.0f,  1.0f,
			1.0f, -1.0f,  1.0f
		};
#pragma endregion
	};

}

