#pragma once

#include "PostProcessing/BloomEffect.h"
#include "PostProcessing/ColorCorrection.h"
#include "PostProcessing/DepthOfField.h"
#include "PostProcessing/PostProcessingStack.h"
#include "REON/Events/EventBus.h"
#include "REON/Events/KeyEvent.h"
#include "REON/Events/RenderEvent.h"
#include "REON/GameHierarchy/Components/Renderer.h"
#include "REON/Platform/Vulkan/VulkanContext.h"
#include "REON/Rendering/LightManager.h"
#include "REON/ResourceManagement/ResourceManager.h"
#include "RenderPasses/DirectionalShadowPass.h"
#include "RenderPasses/TransparentPass.h"
#include "RenderPasses/UnlitPass.h"
#include "vulkan/vulkan.h"
#include <set>

#define MAX_CAMERA_COUNT 10

namespace REON
{
class GameObject;
class EditorCamera;

enum RenderMode
{
    LIT = 0,
    UNLIT = 1,
    WIREFRAME = 2
};

struct CameraSwapChainResources
{
    VkDescriptorSet endDescriptorSet;
    VkImage endImage;
    VmaAllocation endImageAllocation;
    VkImageView endImageView;
    VkFramebuffer framebuffer;

    VkImage colorResolveImage;
    VmaAllocation colorResolveImageAllocation;
    VkImageView colorResolveImageView;

    VkImage depthResolveImage;
    VmaAllocation depthResolveAllocation;
    VkImageView depthResolveView;

    VkImage msaaColorImage;
    VmaAllocation msaaColorAllocation;
    VkImageView msaaColorView;

    VkImage msaaDepthImage;
    VmaAllocation msaaDepthAllocation;
    VkImageView msaaDepthView;
};

struct CameraData
{
    VkBuffer globalBuffer;
    VmaAllocation globalBufferAllocation;
    void* globalBufferMapped;
    VkDescriptorSet globalDescriptorSet;

    VkCommandBuffer commandBuffer;
};

struct FrameData
{
    // Holds the global render data for the current frame per camera
    std::unordered_map<std::shared_ptr<Camera>, CameraData> cameraData =
        std::unordered_map<std::shared_ptr<Camera>, CameraData>(MAX_CAMERA_COUNT);
    VkDescriptorSet lightDescriptorSet;
    VkBuffer lightDataBuffer;
    VmaAllocation lightDataBufferAllocation;
    void* lightDataBufferMapped;
};

using DrawCommandByShaderMaterial =
    std::unordered_map<std::string, std::unordered_map<std::string, std::vector<DrawCommand>>>;

class RenderManager
{
  public:
    void Render(std::shared_ptr<Camera> camera);
    void preRender();
    void AddRenderer(const std::shared_ptr<Renderer>& renderer);
    void RemoveRenderer(std::shared_ptr<Renderer> renderer);
    RenderManager(std::shared_ptr<LightManager> lightManager, std::shared_ptr<EditorCamera> camera);
    void Initialize();
    void HotReloadShaders();
    VkDescriptorSet GetEndBuffer(std::shared_ptr<Camera> camera);
    void SetRenderDimensions(std::shared_ptr<Camera> camera, int width, int height);
    int GetRenderWidth();
    int GetRenderHeight();
    static void InitializeFboAndTexture(uint& fbo, uint& texture, int width, int height);
    static void RenderFullScreenQuad();
    void OnKeyPressed(const KeyPressedEvent& event);
    void OnMaterialChanged(const MaterialChangedEvent& event);
    void cleanup();

    void setMainLight(std::shared_ptr<Light> light);

    RenderMode renderMode = LIT;

  private:
    void RenderOpaques(std::shared_ptr<Camera> camera);
    void RenderTransparents(std::shared_ptr<Camera> camera);
    void RenderPostProcessing();
    void GenerateShadows();
    void GenerateMainLightShadows();
    void GenerateAdditionalShadows();
    void RenderSkyBox();
    void InitializeSkyBox();
    std::vector<LightData> GetLightingBuffer();
    void setGlobalData(std::shared_ptr<Camera> camera);
    void prepareDrawCommands(int currentFrame);

    void deleteForResize(std::shared_ptr<Camera> camera);

    void createSyncObjects();
    void createDummyResources();
    void createEndBufferSet(std::shared_ptr<Camera> camera);

    void createCameraResources(std::shared_ptr<Camera> camera);
    void createOpaqueCommandPool();
    void createOpaqueCommandBuffers(std::shared_ptr<Camera> camera);
    void createOpaqueImages(std::shared_ptr<Camera> camera);
    void createOpaqueRenderPass();
    void createOpaqueFrameBuffers(std::shared_ptr<Camera> camera);
    void createOpaqueDescriptorSetLayout();
    void createOpaqueGlobalDescriptorSets(std::shared_ptr<Camera> camera);
    void createGlobalBuffers(std::shared_ptr<Camera> camera);
    void createOpaqueMaterialDescriptorSets(Material& material);
    void createOpaqueObjectDescriptorSets(const std::shared_ptr<Renderer>& renderer);
    void createOpaqueGraphicsPipelines();
    void createPipelineCache();
    void createEndImages(std::shared_ptr<Camera> camera);
    VkPipeline getPipelineFromFlags(uint32_t flags);
    VkPipeline createGraphicsPipeline(VkPipeline basePipeline, uint32_t flags);

  private:
    const VulkanContext* m_Context;

    VkPipelineCache m_PipelineCache; // TODO: STORE THIS ON DISK

    PFN_vkCmdSetPolygonModeEXT vkCmdSetPolygonModeEXT;

    std::unordered_map<uint32_t, VkPipeline> m_PipelineByMaterialPermutation;
    DrawCommandByShaderMaterial m_DrawCommandsByShaderMaterial;
    std::set<std::string> materials;

    std::vector<FrameData> m_FrameData;
    std::unordered_map<std::shared_ptr<Camera>, std::vector<CameraSwapChainResources>> m_SwapChainResourcesByCamera;

    VkDescriptorSetLayout m_EndDescriptorSetLayout; //
    VkSampler m_EndSampler;

    // OPAQUE PIPELINE
    VkCommandPool m_OpaqueCommandPool;                         //
    VkRenderPass m_OpaqueRenderPass;                           //
    VkDescriptorSetLayout m_OpaqueGlobalDescriptorSetLayout;   //
    VkDescriptorSetLayout m_OpaqueMaterialDescriptorSetLayout; //
    VkDescriptorSetLayout m_OpaqueObjectDescriptorSetLayout;   //
    VkPipelineLayout m_OpaquePipelineLayout;                   //
    VkPipeline m_OpaqueGraphicsPipeline;                       //

    // Directional Shadows
    DirectionalShadowPass m_DirectionalShadowPass;
    std::vector<VkSemaphore> m_DirectionalShadowsGenerated;

    TransparentPass m_TransparentPass;
    std::vector<VkSemaphore> m_OpaquePassDone;

    UnlitPass m_UnlitPass;

    bool resized = false;

    VkImage m_DummyImage;
    VkImageView m_DummyImageView;
    VmaAllocation m_DummyImageAllocation;
    VkSampler m_DummySampler;

    // OLD (some still used, but new things (vulkan) are above this, will filter out whats not used anymore once i get
    // everything working)
    CallbackID m_KeyPressedCallbackID;

    std::unordered_map<std::shared_ptr<Shader>, std::vector<std::shared_ptr<Renderer>>> m_ShaderToRenderer;
    std::vector<std::shared_ptr<Renderer>> m_Renderers;
    std::shared_ptr<EditorCamera> m_Camera;

    // Lighting
    std::shared_ptr<LightManager> m_LightManager;
    glm::mat4 m_MainLightView;
    glm::mat4 m_MainLightProj;
    std::vector<std::shared_ptr<Light>> m_PointLights;

    // Shadows
    uint m_DepthMapFBO;
    uint m_DepthMap;
    const uint MAIN_SHADOW_WIDTH = 4096, MAIN_SHADOW_HEIGHT = 4096;
    const uint ADDITIONAL_SHADOW_WIDTH = 1024, ADDITIONAL_SHADOW_HEIGHT = 1024;
    std::vector<int> m_DepthCubeMaps;
    std::vector<uint> m_AdditionalDepthFBOs;
    std::shared_ptr<Shader> m_DirectionalShadowShader =
        nullptr; // ResourceManager::GetInstance().LoadResource<Shader>("DirectionalShadowShader",
                 // std::make_tuple("DirectionalShadow.vert", "DirectionalShadow.frag", std::optional<std::string>{}));
    std::shared_ptr<Shader> m_AdditionalShadowShader =
        nullptr; // ResourceManager::GetInstance().LoadResource<Shader>("OmnidirectionalShadowShader",
                 // std::make_tuple("OmnidirectionalShadow.vert", "OmnidirectionalShadow.frag",
                 // std::optional<std::string>{"OmnidirectionalShadow.geom"}));

    // Skybox
    uint m_SkyboxVAO, m_SkyboxVBO;
    uint m_SkyboxTexture;
    uint m_HdrTexture;
    uint m_IrradianceMap;
    uint m_PrefilterMap;
    uint m_BrdfLUTTexture;
    // std::shared_ptr<Shader> m_IrradianceShader =
    // ResourceManager::GetInstance().LoadResource<Shader>("IrradianceShader", std::make_tuple("CubeProjection.vert",
    // "IrradianceMap.frag", std::optional<std::string>{})); std::shared_ptr<Shader> m_SkyboxShader =
    // ResourceManager::GetInstance().LoadResource<Shader>("SkyboxShader", std::make_tuple("SkyBox.vert", "SkyBox.frag",
    // std::optional<std::string>{})); std::shared_ptr<Shader> m_SkyboxMappingShader =
    // ResourceManager::GetInstance().LoadResource<Shader>("SkyboxMappingShader", std::make_tuple("CubeProjection.vert",
    // "CubeProjection.frag", std::optional<std::string>{})); std::shared_ptr<Shader> m_PreFilterShader =
    // ResourceManager::GetInstance().LoadResource<Shader>("PreFilterShader", std::make_tuple("CubeProjection.vert",
    // "PreFilter.frag", std::optional<std::string>{})); std::shared_ptr<Shader> m_BrdfShader =
    // ResourceManager::GetInstance().LoadResource<Shader>("BrdfShader", std::make_tuple("brdf.vert", "brdf.frag",
    // std::optional<std::string>{}));
    std::string m_SkyboxLocation = "Assets/Textures/rogland_clear_night_4k.hdr";

    // Screen shader
    uint m_SceneFbo, m_SceneTexture, m_SceneDepthTex;

  public:
    static uint QuadVAO;

  private:
    uint m_QuadVBO;
    float quadVertices[24] = { // vertex attributes for a quad that fills the entire screen in Normalized Device
                               // Coordinates. positions   // texCoords
        -1.0f, 1.0f, 0.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, -1.0f, 1.0f, 0.0f,

        -1.0f, 1.0f, 0.0f, 1.0f, 1.0f,  -1.0f, 1.0f, 0.0f, 1.0f, 1.0f,  1.0f, 1.0f};

    // Post-processing
    PostProcessingStack m_PostProcessingStack;
    uint m_PostProcessTexture, m_PostProcessFbo;
    uint m_RenderResultTexture;

  public:
    static std::shared_ptr<BloomEffect> m_BloomEffect;
    static std::shared_ptr<ColorCorrection> m_ColorCorrection;
    static std::shared_ptr<DepthOfField> m_DepthOfField;

  private:
#pragma region data
    float skyboxVertices[108] = {
        // positions
        -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f,
        1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f,

        -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f,
        -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,

        1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f,

        -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,

        -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f,

        -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f,
        1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f};
#pragma endregion
};

} // namespace REON
