#include "Renderer.h"
#include "reonpch.h"

#include "REON/Application.h"
#include "REON/EditorCamera.h"
#include "REON/GameHierarchy/Components/Light.h"
#include "REON/GameHierarchy/Components/Transform.h"
#include "REON/GameHierarchy/GameObject.h"
#include "REON/GameHierarchy/Scene.h"
#include "REON/Platform/Vulkan/VulkanContext.h"

namespace REON
{

glm::mat4 Renderer::ViewProjMatrix;

void Renderer::update(float deltaTime)
{
    if (m_Transform == nullptr)
    {
        m_Transform = get_owner()->GetTransform();
    }
    m_ModelMatrix = m_Transform->GetWorldTransform();
    m_TransposeInverseModelMatrix = glm::transpose(glm::inverse(m_ModelMatrix));
}

void Renderer::set_owner(std::shared_ptr<GameObject> owner)
{
    Component::set_owner(owner);
    get_owner()->GetScene()->renderManager->AddRenderer(shared_from_this());
}

void Renderer::cleanup()
{
    const VulkanContext* context = static_cast<const VulkanContext*>(Application::Get().GetRenderContext());

    // vkFreeDescriptorSets(context->getDevice(), context->getDescriptorPool(), objectDescriptorSets.size(),
    // objectDescriptorSets.data());

    for (int i = 0; i < context->MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroyBuffer(context->getDevice(), objectDataBuffers[i], nullptr);
        vmaUnmapMemory(context->getAllocator(), objectDataBufferAllocations[i]);
        vmaFreeMemory(context->getAllocator(), objectDataBufferAllocations[i]);

        vkDestroyBuffer(context->getDevice(), shadowObjectDataBuffers[i], nullptr);
        vmaUnmapMemory(context->getAllocator(), shadowObjectDataBufferAllocations[i]);
        vmaFreeMemory(context->getAllocator(), shadowObjectDataBufferAllocations[i]);
    }
}

Renderer::Renderer(ResourceHandle<Mesh> mesh, std::vector<ResourceHandle<Material>> materials)
    : mesh(std::move(mesh)), materials(materials)
{
    const VulkanContext* context = static_cast<const VulkanContext*>(Application::Get().GetRenderContext());

    VkDeviceSize bufferSize = sizeof(ObjectRenderData);

    objectDataBuffers.resize(context->MAX_FRAMES_IN_FLIGHT);
    objectDataBufferAllocations.resize(context->MAX_FRAMES_IN_FLIGHT);
    objectDataBuffersMapped.resize(context->MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < context->MAX_FRAMES_IN_FLIGHT; i++)
    {
        // TODO: use the VMA_ALLOCATION_CREATE_MAPPED_BIT along with VmaAllocationInfo object instead
        context->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                              VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, objectDataBuffers[i],
                              objectDataBufferAllocations[i]);

        vmaMapMemory(context->getAllocator(), objectDataBufferAllocations[i], &objectDataBuffersMapped[i]);
    }

    bufferSize = sizeof(glm::mat4);

    shadowObjectDataBuffers.resize(context->MAX_FRAMES_IN_FLIGHT);
    shadowObjectDataBufferAllocations.resize(context->MAX_FRAMES_IN_FLIGHT);
    shadowObjectDataBuffersMapped.resize(context->MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < context->MAX_FRAMES_IN_FLIGHT; i++)
    {
        // TODO: use the VMA_ALLOCATION_CREATE_MAPPED_BIT along with VmaAllocationInfo object instead
        context->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                              VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, shadowObjectDataBuffers[i],
                              shadowObjectDataBufferAllocations[i]);

        vmaMapMemory(context->getAllocator(), shadowObjectDataBufferAllocations[i], &shadowObjectDataBuffersMapped[i]);
    }
}

Renderer::~Renderer() {}

void Renderer::on_game_object_added_to_scene()
{
    // GetOwner()->GetScene()->renderManager->AddRenderer(shared_from_this());
}

void Renderer::on_component_detach()
{
    get_owner()->GetScene()->renderManager->RemoveRenderer(shared_from_this());
}

void Renderer::RebuildDrawCommands()
{
    drawCommands.clear();

    if (!mesh.Lock())
        return;

    //TODO: make this take shared or weak pointers (if shared make sure to destroy after each frame to prevent them not freeing)

    for (auto submesh : mesh.Lock()->subMeshes)
    {
        if (submesh.materialIndex >= materials.size() || submesh.materialIndex < 0)
            continue;

        auto mat = materials[submesh.materialIndex];
        DrawCommand cmd;
        cmd.mesh = mesh;
        cmd.material = mat;
        cmd.shader = {};
        cmd.startIndex = submesh.indexOffset;
        cmd.indexCount = submesh.indexCount;
        cmd.owner = this;

        drawCommands.push_back(cmd);
    }
}
void Renderer::MarkDrawCommandsDirty()
{
    drawCommandsDirty = true;
}
void Renderer::SetMaterial(size_t index, const ResourceHandle<Material>& material)
{
    if (index >= materials.size())
    {
        REON_CORE_ERROR("Tried to set material of renderer {}, but index: {}, is outside of material list size: {}",
                        GetName(), index, materials.size());
        return;
    }

    materials[index] = material;
    MarkDrawCommandsDirty();
}
} // namespace REON