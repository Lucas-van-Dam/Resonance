#include "reonpch.h"
#include "RigLoader.h"

#include "REON/AssetManagement/ModelBinFormat.h"
#include "REON/Rendering/Animation/Rig.h"

namespace REON
{
std::shared_ptr<ResourceBase> RigLoader::Load(const AssetKey& key, const ArtifactRef& ref, IBlobReader& reader)
{
    std::vector<std::byte> bytes;
    if (!reader.ReadRange(ref.uri, ref.offset, ref.size, bytes))
        return {};

    if (bytes.size() < sizeof(RigChunkHeader))
        return {};

    RigChunkHeader h{};
    std::memcpy(&h, bytes.data(), sizeof(h));

    if (h.version != 1)
        return {};

    const uint8_t* base = reinterpret_cast<const uint8_t*>(bytes.data());
    const size_t size = bytes.size();

    const size_t skinsTableBytes = size_t(h.skinCount) * sizeof(SkinRecord);

    if (h.skinOffset == 0)
        return {}; // must exist if you embed skins
    if (size_t(h.skinOffset) + skinsTableBytes > size)
        return {};

    std::vector<SkinRecord> skins(h.skinCount);
    std::memcpy(skins.data(), base + h.skinOffset, skinsTableBytes);

    std::vector<AssetId> rigJointIds;
    rigJointIds.resize(h.jointCount);

    if (h.jointCount > 0)
    {
        const size_t idsBytes = size_t(h.jointCount) * 16;

        if (h.jointNodeIdsOffset == 0)
            return {};
        if (size_t(h.jointNodeIdsOffset) + idsBytes > size)
            return {};

        std::memcpy(rigJointIds.data(), base + h.jointNodeIdsOffset, idsBytes);
    }

    // ---- skin record table ----
    const size_t recBytes = size_t(h.skinCount) * sizeof(SkinRecord);

    if (h.skinOffset == 0)
        return {};
    if (size_t(h.skinOffset) + recBytes > size)
        return {};

    std::vector<SkinRecord> recs(h.skinCount);
    if (!recs.empty())
        std::memcpy(recs.data(), base + h.skinOffset, recBytes);

    auto rig = std::make_shared<Rig>();
    rig->skins.reserve(h.skinCount);

    rig->joints.reserve(h.jointCount);

    for (const auto& joint : rigJointIds)
    {
        Rig::Joint j;
        j.nodeId = joint;
        rig->joints.push_back(j);
    }

    for (uint32_t i = 0; i < h.skinCount; ++i)
    {
        const SkinRecord& sr = recs[i];

        const size_t jointIdxBytes = size_t(sr.jointCount) * sizeof(uint32_t);
        const size_t ibmBytes = size_t(sr.jointCount) * sizeof(glm::mat4);

        if (size_t(sr.jointsOffset) + jointIdxBytes > size)
            return {};
        if (size_t(sr.ibmOffset) + ibmBytes > size)
            return {};

        const uint32_t* jointIdx = reinterpret_cast<const uint32_t*>(base + sr.jointsOffset);

        Rig::Skin skin{};
        skin.name = "Skin_" + std::to_string(i);

        if (sr.skeletonNode != UINT32_MAX)
            skin.skeletonRootJoint = sr.skeletonNode;
        else
            skin.skeletonRootJoint.reset();

        skin.jointIdx.reserve(sr.jointCount);
        for (uint32_t j = 0; j < sr.jointCount; ++j)
        {
            const uint32_t paletteIndex = jointIdx[j];
            if (paletteIndex >= rigJointIds.size())
                return {}; // corrupted or mismatch between rigJointIds and joint indices

            skin.jointIdx.push_back(paletteIndex);
        }

        skin.inverseBindMatrices.resize(sr.jointCount);
        if (sr.jointCount > 0)
        {
            std::memcpy(skin.inverseBindMatrices.data(), base + sr.ibmOffset, ibmBytes);
        }

        rig->skins.push_back(std::move(skin));
    }

    return rig;
}
} // namespace REON