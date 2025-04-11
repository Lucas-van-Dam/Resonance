#include "reonpch.h"
#include "RendererComponent.h"

#include "REON/GameHierarchy/GameObject.h"
#include "REON/GameHierarchy/Scene.h"
#include "REON/GameHierarchy/Components/Transform.h"
#include "REON/GameHierarchy/Components/Light.h"


namespace REON {

    void RendererComponent::Draw(glm::mat4 mainLightView, glm::mat4 mainLightProj, int skyboxId, int irradianceMapId, int prefilterMapId, int brdfLUTTextureId, std::vector<int> depthCubeId, int shadowMapId,
        const std::shared_ptr<Shader>&overrideShader) {
        auto data = SetLightingBuffer(mainLightView, mainLightProj);
        if (overrideShader != nullptr) {
            overrideShader->use();
            overrideShader->setMat4("model", m_ModelMatrix);
            overrideShader->setMat4("view", m_ViewMatrix);
            overrideShader->setMat4("projection", m_ProjectionMatrix);
            Material overrideMaterial(overrideShader);
            mesh.Get<Mesh>()->Draw(overrideMaterial, data);
            return;
        }
        std::shared_ptr<Material> mat = material.Get<Material>();
        mat->shader->setMat4("model", m_ModelMatrix);
        mat->shader->setMat4("view", m_ViewMatrix);
        mat->shader->setMat4("projection", m_ProjectionMatrix);
        mat->shader->setFloat("far_plane", 100.0f);
        glActiveTexture(GL_TEXTURE4);
        glUniform1i(glGetUniformLocation(mat->shader->ID, "shadowMap"), 4);
        glBindTexture(GL_TEXTURE_2D, shadowMapId);

        if (skyboxId >= 0) {
            glActiveTexture(GL_TEXTURE5);
            glUniform1i(glGetUniformLocation(mat->shader->ID, "skyboxCube"), 5);
            glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxId);

            glActiveTexture(GL_TEXTURE6);
            glUniform1i(glGetUniformLocation(mat->shader->ID, "irradianceMap"), 6);
            glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMapId);

            glActiveTexture(GL_TEXTURE7);
            glUniform1i(glGetUniformLocation(mat->shader->ID, "preFilterMap"), 7);
            glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMapId);

            glActiveTexture(GL_TEXTURE8);
            glUniform1i(glGetUniformLocation(mat->shader->ID, "brdfLUT"), 8);
            glBindTexture(GL_TEXTURE_2D, brdfLUTTextureId);
        }

        for (int i = 0; i < depthCubeId.size(); i++) {
            glBindTextureUnit(9 + i, depthCubeId[i]);
        }

        mesh.Get<Mesh>()->Draw(*mat, data);
    }

    void RendererComponent::Update(float deltaTime) {
        if (m_Transform == nullptr) {
            m_Transform = GetOwner()->GetTransform();
        }
        m_ModelMatrix = m_Transform->GetWorldTransform();
        m_ViewMatrix = GetOwner()->GetScene()->GetEditorCamera()->GetViewMatrix();
        m_ProjectionMatrix = GetOwner()->GetScene()->GetEditorCamera()->GetProjectionMatrix();
    }

    void RendererComponent::SetOwner(std::shared_ptr<GameObject> owner)
    {
        Component::SetOwner(owner);
        GetOwner()->GetScene()->renderManager->AddRenderer(shared_from_this());
    }

    RendererComponent::RendererComponent(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material) : mesh(std::move(mesh)), material(material)
    {
        
    }

    RendererComponent::~RendererComponent()
    {
        
    }

    std::vector<LightData> RendererComponent::SetLightingBuffer(glm::mat4 mainLightView, glm::mat4 mainLightProj) {
        size_t amountOfLights = GetOwner()->GetScene()->lightManager->lights.size();
        int pointIndex = 0;
        std::vector<LightData> lights;
        for (int i = 0; i < amountOfLights; ++i) {
            unsigned int depthCube = -1;
            Light* light = GetOwner()->GetScene()->lightManager->lights[i].get();

            LightData data(light->intensity, light->color, light->GetOwner()->GetTransform()->localPosition,
                light->GetOwner()->GetTransform()->GetForwardVector(), light->innerCutOff, light->outerCutOff, (int)light->type, mainLightProj, mainLightView);
            lights.emplace_back(data);
        }
        return lights;
    }

    void RendererComponent::OnGameObjectAddedToScene() {
        //GetOwner()->GetScene()->renderManager->AddRenderer(shared_from_this());
    }

    void RendererComponent::OnComponentDetach()
    {
        GetOwner()->GetScene()->renderManager->RemoveRenderer(shared_from_this());
    }


}