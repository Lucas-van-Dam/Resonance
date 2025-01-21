#pragma once

#include "REON/GameHierarchy/Components/Renderer.h"
#include "REON/Rendering/LightManager.h"
#include "REON/ResourceManagement/ResourceManager.h"



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
        unsigned int GetEndBuffer();
        void SetRenderDimensions(int width, int height);
        int GetRenderWidth();
        int GetRenderHeight();

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
        unsigned int m_DepthMapFBO;
        unsigned int m_DepthMap;
        const unsigned int MAIN_SHADOW_WIDTH = 4096, MAIN_SHADOW_HEIGHT = 4096;
        const unsigned int ADDITIONAL_SHADOW_WIDTH = 1024, ADDITIONAL_SHADOW_HEIGHT = 1024;
        std::vector<int> m_DepthCubeMaps;
        std::vector<unsigned int> m_AdditionalDepthFBOs;
        std::shared_ptr<Shader> m_DirectionalShadowShader = ResourceManager::GetInstance().LoadResource<Shader>("DirectionalShadowShader", std::make_tuple("DirectionalShadow.vert", "DirectionalShadow.frag", std::optional<std::string>{}));
        std::shared_ptr<Shader> m_AdditionalShadowShader = ResourceManager::GetInstance().LoadResource<Shader>("OmnidirectionalShadowShader", std::make_tuple("OmnidirectionalShadow.vert", "OmnidirectionalShadow.frag", std::optional<std::string>{"OmnidirectionalShadow.geom"}));

        //Skybox
        unsigned int m_SkyboxVAO, m_SkyboxVBO;
        unsigned int m_SkyboxTexture;
        unsigned int m_HdrTexture;
        unsigned int m_IrradianceMap;
        unsigned int m_PrefilterMap;
        unsigned int m_BrdfLUTTexture;
        std::shared_ptr<Shader> m_IrradianceShader = ResourceManager::GetInstance().LoadResource<Shader>("IrradianceShader", std::make_tuple("CubeProjection.vert", "IrradianceMap.frag", std::optional<std::string>{}));
        std::shared_ptr<Shader> m_SkyboxShader = ResourceManager::GetInstance().LoadResource<Shader>("SkyboxShader", std::make_tuple("SkyBox.vert", "SkyBox.frag", std::optional<std::string>{}));
        std::shared_ptr<Shader> m_SkyboxMappingShader = ResourceManager::GetInstance().LoadResource<Shader>("SkyboxMappingShader", std::make_tuple("CubeProjection.vert", "CubeProjection.frag", std::optional<std::string>{}));
        std::shared_ptr<Shader> m_PreFilterShader = ResourceManager::GetInstance().LoadResource<Shader>("PreFilterShader", std::make_tuple("CubeProjection.vert", "PreFilter.frag", std::optional<std::string>{}));
        std::shared_ptr<Shader> m_BrdfShader = ResourceManager::GetInstance().LoadResource<Shader>("BrdfShader", std::make_tuple("brdf.vert", "brdf.frag", std::optional<std::string>{}));
        std::string m_SkyboxLocation = "Assets/Textures/Grey Outside.jpg";

        //Screen shader
        unsigned int m_Framebuffer, m_TextureColorbuffer, m_Rbo;
        unsigned int m_QuadVAO, m_QuadVBO;
        std::shared_ptr<Shader> m_ScreenShader = ResourceManager::GetInstance().LoadResource<Shader>("ScreenShader", std::make_tuple("fullScreen.vert", "fullScreen.frag", std::optional<std::string>{}));
        float quadVertices[24] = { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
            // positions   // texCoords
            -1.0f,  1.0f,  0.0f, 1.0f,
            -1.0f, -1.0f,  0.0f, 0.0f,
            1.0f, -1.0f,  1.0f, 0.0f,

            -1.0f,  1.0f,  0.0f, 1.0f,
            1.0f, -1.0f,  1.0f, 0.0f,
            1.0f,  1.0f,  1.0f, 1.0f
        };

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

