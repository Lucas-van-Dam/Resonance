#pragma once
#include "ModelBinContainerReader.h"
#include "REON/AssetManagement/BlobIO.h"

#include <vector>

namespace REON
{
class SceneLoader
{
  public:
    static bool LoadSceneNodes(ModelBinContainerReader& model, IBlobReader& reader, std::vector<SceneNode>& outNodes)
    {
        uint64_t off = 0, sz = 0;
        if (!model.GetChunkSlice(ChunkType::SCENE, off, sz))
            return false;

        std::vector<std::byte> bytes;
        if (!reader.ReadRange(model.ModelRef().uri, off, sz, bytes))
            return false;

        uint32_t nodeCount = 0;
        std::memcpy(&nodeCount, bytes.data(), sizeof(uint32_t));

        const uint64_t nodesBytes = sz - sizeof(uint32_t);
        if (nodesBytes != uint64_t(nodeCount) * sizeof(SceneNode))
            return false;

        const SceneNode* nodes =
            reinterpret_cast<const SceneNode*>(reinterpret_cast<const uint8_t*>(bytes.data()) + sizeof(uint32_t));

        outNodes.assign(nodes, nodes + nodeCount);
        return true;
    }
};
} // namespace REON