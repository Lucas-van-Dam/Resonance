#include "reonpch.h"

#include "Renderer.h"

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
}

Renderer::Renderer(ResourceHandle<Mesh> mesh, std::vector<ResourceHandle<Material>> materials)
    : mesh(std::move(mesh)), materials(materials)
{
    const VulkanContext* context = static_cast<const VulkanContext*>(Application::Get().GetRenderContext());

    VkDeviceSize bufferSize = sizeof(ObjectRenderData);

    objectDataBuffers.resize(context->MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < context->MAX_FRAMES_IN_FLIGHT; i++)
    {
        BufferCreateInfo bufCreateInfo;
        bufCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        bufCreateInfo.cpuAccess = CpuAccessPattern::SequentialWrite;
        bufCreateInfo.memoryHint = BufferMemoryHint::CpuToGpu;
        bufCreateInfo.persistentlyMapped = true;
        bufCreateInfo.size = bufferSize;

        objectDataBuffers[i] = context->createBuffer(bufCreateInfo);
    }

    bufferSize = sizeof(glm::mat4);

    shadowObjectDataBuffers.resize(context->MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < context->MAX_FRAMES_IN_FLIGHT; i++)
    {
        BufferCreateInfo bufCreateInfo;
        bufCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        bufCreateInfo.cpuAccess = CpuAccessPattern::SequentialWrite;
        bufCreateInfo.memoryHint = BufferMemoryHint::CpuToGpu;
        bufCreateInfo.persistentlyMapped = true;
        bufCreateInfo.size = bufferSize;

        shadowObjectDataBuffers[i] = context->createBuffer(bufCreateInfo);
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
        if (m_SkinIndex)
        {
            cmd.joinOffset = 0;
            cmd.jointCount = animator->get_amount_of_joints(m_SkinIndex.value());
        }

        drawCommands.push_back(cmd);
    }

    drawCommandsDirty = false;
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