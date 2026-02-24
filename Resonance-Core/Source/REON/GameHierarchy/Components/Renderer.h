#pragma once

#include <memory>
#include <vector>

#include "REON/GameHierarchy/Components/Component.h"
#include "REON/Rendering/Material.h"
#include "REON/Rendering/Mesh.h"
#include "REON/Rendering/Shader.h"
#include "REON/Rendering/Structs/LightData.h"
#include "glm/glm.hpp"
#include <REON/Rendering/LightManager.h>

namespace REON
{

class Transform;
class GameObject;
class EditorCamera;
struct DrawCommand;

struct alignas(16) GlobalRenderData
{
    glm::mat4 viewProj;
    glm::mat4 inverseView;
    int lightCount;
    glm::vec3 _padding;
};

struct alignas(16) ObjectRenderData
{
    glm::mat4 model;
    glm::mat4 transposeInverseModel;
};

class [[clang::annotate("serialize")]] Renderer : public ComponentBase<Renderer>,
                                                  public std::enable_shared_from_this<Renderer>
{
  public:
    static_assert(sizeof(GlobalRenderData) % 16 == 0, "Global is not aligned");
    Renderer(ResourceHandle<Mesh> mesh, std::vector<ResourceHandle<Material>> materials);
    ~Renderer();
    Renderer() {} // Delete later, just to prevent errors during migration
    void update(float deltaTime) override;

    virtual void set_owner(std::shared_ptr<GameObject> owner) override;

    void cleanup() override;

    glm::mat4 getModelMatrix()
    {
        return m_ModelMatrix;
    }
    glm::mat4 getTransposeInverseModelMatrix()
    {
        return m_TransposeInverseModelMatrix;
    }

    virtual void on_game_object_added_to_scene() override;
    virtual void on_component_detach() override;

    void RebuildDrawCommands();
    void MarkDrawCommandsDirty();

    void SetMaterial(size_t index, const ResourceHandle<Material>& material);

    [[nodiscard]] nlohmann::ordered_json serialize() const override
    {
        return {};
    }
    void deserialize(const nlohmann::ordered_json& json, std::filesystem::path basePath) override {}

  public:
    ResourceHandle<Mesh> mesh;
    std::vector<ResourceHandle<Material>> materials;

    static glm::mat4 ViewProjMatrix;

    std::vector<DrawCommand> drawCommands;
    bool drawCommandsDirty = true;

    std::vector<VkDescriptorSet> objectDescriptorSets;
    std::vector<VkBuffer> objectDataBuffers;
    std::vector<VmaAllocation> objectDataBufferAllocations;
    std::vector<void*> objectDataBuffersMapped;

    std::vector<VkDescriptorSet> shadowObjectDescriptorSets;
    std::vector<VkBuffer> shadowObjectDataBuffers;
    std::vector<VmaAllocation> shadowObjectDataBufferAllocations;
    std::vector<void*> shadowObjectDataBuffersMapped;

  private:
    glm::mat4 m_ModelMatrix{};
    glm::mat4 m_TransposeInverseModelMatrix{};

    std::shared_ptr<Transform> m_Transform = nullptr;

    template <typename ClassType, typename FieldType, FieldType ClassType::* field> friend struct ReflectionAccessor;
};

struct DrawCommand
{
    ResourceHandle<Shader> shader;
    ResourceHandle<Material> material;
    ResourceHandle<Mesh> mesh;
    uint32_t startIndex;
    uint32_t indexCount;
    Renderer* owner;
};

} // namespace REON
