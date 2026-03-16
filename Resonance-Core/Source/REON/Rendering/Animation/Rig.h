#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "REON/AssetManagement/Asset.h"
#include "REON/ResourceManagement/Resource.h"

namespace REON
{
struct Rig : public ResourceBase
{
    static constexpr AssetTypeId kType = ASSET_RIG;
    
    struct Joint
    {
        AssetId nodeId;
        int parentIndex; // -1 for root
    };

    std::vector<Joint> joints;

    struct Skin
    {
        std::string name;

        std::vector<uint32_t> jointIdx;

        std::vector<glm::mat4> inverseBindMatrices;

        std::optional<uint32_t> skeletonRootJoint;
    };

    std::vector<Skin> skins;
};
} // namespace REON