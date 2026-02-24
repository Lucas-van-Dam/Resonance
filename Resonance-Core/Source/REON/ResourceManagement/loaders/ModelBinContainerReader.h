#pragma once

#include "REON/AssetManagement/Artifact.h"
#include "REON/AssetManagement/BlobIO.h"
#include "REON/AssetManagement/ModelBinFormat.h"
#include <vector>

namespace REON
{

class ModelBinContainerReader
{
  public:
    bool Open(const ArtifactRef& modelRef, IBlobReader& reader);

    const ChunkEntry* FindChunk(ChunkType type) const
    {
        for (const auto& c : chunks_)
            if (c.type == type)
                return &c;
        return nullptr;
    }

    // Returns absolute file offsets for chunk payload.
    bool GetChunkSlice(ChunkType type, uint64_t& outAbsOffset, uint64_t& outSize) const
    {
        const ChunkEntry* c = FindChunk(type);
        if (!c)
            return false;

        // ChunkEntry.offset is assumed to be file-relative (from file start).
        outAbsOffset = modelRef_.offset + c->offset;
        outSize = c->size;
        return true;
    }

    const FileHeader& Header() const
    {
        return header_;
    }
    const ArtifactRef& ModelRef() const
    {
        return modelRef_;
    }

  private:
    ArtifactRef modelRef_{};
    FileHeader header_{};
    std::vector<ChunkEntry> chunks_;
};
}