#include "reonpch.h"

#include "Animator.h"

#include "REON/GameHierarchy/GameObject.h"

namespace REON
{
Animator::Animator(ResourceHandle<Rig> rig) : m_Rig(rig) {}

void Animator::update(float deltaTime)
{
    auto rig = m_Rig.Lock();
    auto object = get_owner();

    for (int i = 0; i < rig->skins.size(); ++i)
    {
        auto& skin = rig->skins[i];
        auto& bindings = m_SkinJointBindings[i];
        auto& palette = m_SkinPalettes[i];

        for (size_t i = 0; i < bindings.size(); ++i)
        {
            auto joint = bindings[i].lock();
            glm::mat4 jointWorld = joint->GetWorldTransform();

            glm::mat4 jointMesh = object->GetTransform()->GetWorldTransform() * jointWorld;

            palette[i] = jointMesh * skin.inverseBindMatrices[i];
        }
    }
}

void Animator::set_owner(std::shared_ptr<GameObject> owner) 
{
    Component::set_owner(owner);
}

void Animator::on_game_object_added_to_scene()
{
    std::vector<std::shared_ptr<GameObject>> stack;
    stack.push_back(get_owner());

    while (!stack.empty())
    {
        std::shared_ptr<GameObject> current = stack.back();
        stack.pop_back();

        if (auto t = current->GetTransform())
        {
            nodeLookup[current->GetNodeId()] = t;
        }

        for (auto& child : current->GetChildren())
            stack.push_back(child);
    }

    auto rig = m_Rig.Lock();

    m_SkinJointBindings.resize(rig->skins.size());

    m_SkinPalettes.resize(rig->skins.size());

    for (size_t s = 0; s < rig->skins.size(); ++s)
    {
        auto& skin = rig->skins[s];

        m_SkinPalettes[s].resize(skin.jointIdx.size());

        auto& bindings = m_SkinJointBindings[s];
        bindings.resize(skin.jointIdx.size());

        for (size_t j = 0; j < skin.jointIdx.size(); ++j)
        {
            uint32_t rigJointIndex = skin.jointIdx[j];
            AssetId nodeId = rig->joints[rigJointIndex].nodeId;

            bindings[j] = nodeLookup[nodeId];
        }
    }
}
uint32_t Animator::get_amount_of_joints(uint32_t skinIndex)
{
    return m_Rig.Lock()->skins[skinIndex].jointIdx.size();
}
std::vector<glm::mat4> Animator::getPalettes()
{
    std::vector<glm::mat4> mats;
    for (const auto& palette : m_SkinPalettes)
    {
        for (const auto& mat : palette)
        {
            mats.push_back(mat);
        }
    }

    return mats;
}
} // namespace REON