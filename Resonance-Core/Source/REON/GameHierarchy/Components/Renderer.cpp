#include "reonpch.h"
#include "Renderer.h"

#include "REON/GameHierarchy/GameObject.h"
#include "REON/GameHierarchy/Scene.h"
#include "REON/GameHierarchy/Components/Transform.h"
#include "REON/GameHierarchy/Components/Light.h"
#include "REON/Application.h"
#include "REON/Platform/Vulkan/VulkanContext.h"
#include "REON/EditorCamera.h"

namespace REON {

    glm::mat4 Renderer::ViewProjMatrix;

    void Renderer::Draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, VkDescriptorSet globalDescriptorSet) {
        const VulkanContext* context = static_cast<const VulkanContext*>(Application::Get().GetRenderContext());

        std::vector<VkDescriptorSet> descriptorSets;
        descriptorSets.push_back(globalDescriptorSet);
        descriptorSets.push_back(material.Get<Material>()->descriptorSets[context->getCurrentFrame()]);
        descriptorSets.push_back(objectDescriptorSets[context->getCurrentFrame()]);

        ObjectRenderData data{};
        data.model = m_ModelMatrix;
        data.transposeInverseModel = glm::transpose(glm::inverse(m_ModelMatrix));
        memcpy(objectDataBuffersMapped[context->getCurrentFrame()], &data, sizeof(data));

        VkBuffer vertexBuffers[] = { mesh.Get<Mesh>()->m_VertexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

        vkCmdBindIndexBuffer(commandBuffer, mesh.Get<Mesh>()->m_IndexBuffer, 0, VK_INDEX_TYPE_UINT32);
        
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, descriptorSets.size(), descriptorSets.data(), 0, nullptr);
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(mesh.Get<Mesh>()->indices.size()), 1, 0, 0, 0);
    }

    void Renderer::Draw(glm::mat4 mainLightView, glm::mat4 mainLightProj, int skyboxId, int irradianceMapId, int prefilterMapId, int brdfLUTTextureId, std::vector<int> depthCubeId, int shadowMapId,
        const std::shared_ptr<Shader>&overrideShader) {
        /*
        auto data = GetLightingBuffer();
        if (overrideShader != nullptr) {
            overrideShader->use();
            //overrideShader->setMat4("model", m_ModelMatrix);
            //overrideShader->setMat4("view", m_ViewMatrix);
            //overrideShader->setMat4("projection", m_ProjectionMatrix);
            Material overrideMaterial(overrideShader);
            mesh.Get<Mesh>()->Draw(overrideMaterial, data);
            return;
        }
        std::shared_ptr<Material> mat = material.Get<Material>();
        //mat->shader->setMat4("model", m_ModelMatrix);
        //mat->shader->setMat4("view", m_ViewMatrix);
        //mat->shader->setMat4("projection", m_ProjectionMatrix);
        //mat->shader->setFloat("far_plane", 100.0f);
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
            glUniform1i(glGetUniformLocation(mat->shader.get->ID, "brdfLUT"), 8);
            glBindTexture(GL_TEXTURE_2D, brdfLUTTextureId);
        }

        for (int i = 0; i < depthCubeId.size(); i++) {
            glBindTextureUnit(9 + i, depthCubeId[i]);
        }

        mesh.Get<Mesh>()->Draw(*mat, data);
        */
    }

    void Renderer::Update(float deltaTime) {
        if (m_Transform == nullptr) {
            m_Transform = GetOwner()->GetTransform();
        }
        m_ModelMatrix = m_Transform->GetWorldTransform();
    }

    void Renderer::SetOwner(std::shared_ptr<GameObject> owner)
    {
        Component::SetOwner(owner);
        GetOwner()->GetScene()->renderManager->AddRenderer(shared_from_this());
    }

    void Renderer::cleanup()
    {
        const VulkanContext* context = static_cast<const VulkanContext*>(Application::Get().GetRenderContext());

        
        //vkFreeDescriptorSets(context->getDevice(), context->getDescriptorPool(), objectDescriptorSets.size(), objectDescriptorSets.data());

        for (int i = 0; i < context->MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroyBuffer(context->getDevice(), objectDataBuffers[i], nullptr);
            vmaUnmapMemory(context->getAllocator(), objectDataBufferAllocations[i]);
            vmaFreeMemory(context->getAllocator(), objectDataBufferAllocations[i]);
        }
    }

    Renderer::Renderer(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material) : mesh(std::move(mesh)), material(material)
    {
        const VulkanContext* context = static_cast<const VulkanContext*>(Application::Get().GetRenderContext());

        VkDeviceSize bufferSize = sizeof(ObjectRenderData);

        objectDataBuffers.resize(context->MAX_FRAMES_IN_FLIGHT);
        objectDataBufferAllocations.resize(context->MAX_FRAMES_IN_FLIGHT);
        objectDataBuffersMapped.resize(context->MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < context->MAX_FRAMES_IN_FLIGHT; i++) {
            // TODO: use the VMA_ALLOCATION_CREATE_MAPPED_BIT along with VmaAllocationInfo object instead
            context->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, objectDataBuffers[i], objectDataBufferAllocations[i]);

            vmaMapMemory(context->getAllocator(), objectDataBufferAllocations[i], &objectDataBuffersMapped[i]);
        }
    }

    Renderer::Renderer()
    {
        const VulkanContext* context = static_cast<const VulkanContext*>(Application::Get().GetRenderContext());

        VkDeviceSize bufferSize = sizeof(ObjectRenderData);

        objectDataBuffers.resize(context->MAX_FRAMES_IN_FLIGHT);
        objectDataBufferAllocations.resize(context->MAX_FRAMES_IN_FLIGHT);
        objectDataBuffersMapped.resize(context->MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < context->MAX_FRAMES_IN_FLIGHT; i++) {
            // TODO: use the VMA_ALLOCATION_CREATE_MAPPED_BIT along with VmaAllocationInfo object instead
            context->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, objectDataBuffers[i], objectDataBufferAllocations[i]);

            vmaMapMemory(context->getAllocator(), objectDataBufferAllocations[i], &objectDataBuffersMapped[i]);
        }
    }

    Renderer::~Renderer()
    {
        
    }

    void Renderer::OnGameObjectAddedToScene() {
        //GetOwner()->GetScene()->renderManager->AddRenderer(shared_from_this());
    }

    void Renderer::OnComponentDetach()
    {
        GetOwner()->GetScene()->renderManager->RemoveRenderer(shared_from_this());
    }
}