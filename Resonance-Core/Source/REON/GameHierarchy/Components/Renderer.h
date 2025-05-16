#pragma once

#include <memory>
#include <vector>

#include "REON/Rendering/Shader.h"
#include "REON/Rendering/Mesh.h"
#include "REON/Rendering/Material.h"
#include "REON/Rendering/Structs/LightData.h"
#include "REON/GameHierarchy/Components/Component.h"
#include "glm/glm.hpp"
#include <REON/Rendering/LightManager.h>

namespace REON {

    class Transform;
    class GameObject;
    class EditorCamera;

    struct alignas(16) GlobalRenderData {
        glm::mat4 viewProj;
        glm::mat4 inverseView;
        LightData lightData[REON_MAX_LIGHTS];
        int lightCount;
        glm::vec3 _padding;
    };

    struct alignas(16) ObjectRenderData {
        glm::mat4 model;
        glm::mat4 transposeInverseModel;
    };

    class [[clang::annotate("serialize")]] Renderer : public ComponentBase<Renderer>, public std::enable_shared_from_this<Renderer> {
    public:
        static_assert(sizeof(GlobalRenderData)%16 == 0, "Global is not aligned");
        Renderer(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> materials);
        Renderer();
        ~Renderer();
        void Draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, VkDescriptorSet globalDescriptorSet);
        void Draw(glm::mat4 mainLightView, glm::mat4 mainLightProj, int skyboxId, int irradianceMapId, int prefilterMapId, int brdfLUTTextureId, std::vector<int> depthCubeId = std::vector<int>(), int shadowMapId = 0, const std::shared_ptr<Shader>&overrideShader = nullptr);

        void Update(float deltaTime) override;

        void SetOwner(std::shared_ptr<GameObject> owner) override;

        void cleanup() override;

        void OnGameObjectAddedToScene() override;
        void OnComponentDetach() override;

    public:
        ResourceHandle mesh;
        ResourceHandle material;

        static glm::mat4 ViewProjMatrix;


        std::vector<VkDescriptorSet> objectDescriptorSets;
        std::vector<VkBuffer> objectDataBuffers;
        std::vector<VmaAllocation> objectDataBufferAllocations;
        std::vector<void*> objectDataBuffersMapped;

    private:
        glm::mat4 m_ModelMatrix{};



        std::shared_ptr<Transform> m_Transform = nullptr;

        template <typename ClassType, typename FieldType, FieldType ClassType::* field>
        friend struct ReflectionAccessor;
    };

}

