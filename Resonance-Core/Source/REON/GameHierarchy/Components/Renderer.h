#pragma once

#include <memory>
#include <vector>

#include "REON/Rendering/Shader.h"
#include "REON/Rendering/Mesh.h"
#include "REON/Rendering/Material.h"
#include "REON/Rendering/Structs/LightData.h"
#include "REON/GameHierarchy/Components/Component.h"
#include "glm/glm.hpp"



namespace REON {

    class Transform;
    class GameObject;

    class [[clang::annotate("serialize")]] Renderer : public ComponentBase<Renderer>, public std::enable_shared_from_this<Renderer> {
    public:
        Renderer(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> materials);
        Renderer() {}
        ~Renderer();
        void Draw(glm::mat4 mainLightView, glm::mat4 mainLightProj, int skyboxId, int irradianceMapId, int prefilterMapId, int brdfLUTTextureId, std::vector<int> depthCubeId = std::vector<int>(), int shadowMapId = 0, const std::shared_ptr<Shader>&overrideShader = nullptr);

        void Update(float deltaTime) override;

        void SetOwner(std::shared_ptr<GameObject> owner) override;

        void OnGameObjectAddedToScene() override;
        void OnComponentDetach() override;

    public:
        ResourceHandle mesh;
        ResourceHandle material;

    private:
        std::vector<LightData> SetLightingBuffer(glm::mat4 mainLightView, glm::mat4 mainLightProj);

    private:
        glm::mat4 m_ModelMatrix{};
        glm::mat4 m_ViewMatrix{};
        glm::mat4 m_ProjectionMatrix{};

        std::shared_ptr<Transform> m_Transform = nullptr;

        template <typename ClassType, typename FieldType, FieldType ClassType::* field>
        friend struct ReflectionAccessor;
    };

}

