#pragma once

#include "REON/GameHierarchy/Components/Component.h"
#include "REON/Rendering/Animation/Rig.h"
#include "REON/ResourceManagement/Resource.h"
#include "REON/GameHierarchy/Components/Transform.h"

namespace REON
{

class Animator final : public ComponentBase<Animator>
{
  public:
    Animator(ResourceHandle<Rig>);

    virtual void update(float deltaTime) override;

    virtual void cleanup() override {};

    virtual void set_owner(std::shared_ptr<GameObject> owner) override;

    virtual void on_game_object_added_to_scene() override;
    virtual void on_component_detach() override {};

    uint32_t get_amount_of_joints(uint32_t skinIndex);

    std::vector<glm::mat4> getPalettes();

  private:
    ResourceHandle<Rig> m_Rig;

    std::vector<std::vector<glm::mat4>> m_SkinPalettes;

    std::vector<std::vector<std::weak_ptr<Transform>>> m_SkinJointBindings;

    std::unordered_map<AssetId, std::weak_ptr<Transform>> nodeLookup;
};

} // namespace REON