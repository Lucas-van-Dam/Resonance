#include "reonpch.h"

#include "ModelBinContainerReader.h"
#include <REON/AssetManagement/ModelBinFormat.h>

namespace REON
{
bool ModelBinContainerReader::Open(const ArtifactRef& modelRef, IBlobReader& reader)
{

    modelRef_ = modelRef;

    std::vector<std::byte> hdrBytes;
    if (!reader.ReadRange(modelRef_.uri, modelRef_.offset, sizeof(FileHeader), hdrBytes))
        return false;
    if (hdrBytes.size() != sizeof(FileHeader))
        return false;

    std::memcpy(&header_, hdrBytes.data(), sizeof(FileHeader));

    if (header_.magic != FILE_MAGIC)
        return false;
    if (header_.version != FILE_VERSION)
        return false;
    if (header_.endian != 1)
        return false;

    const uint64_t tableOffset = modelRef_.offset + sizeof(FileHeader);
    const uint64_t tableSize = uint64_t(header_.chunkCount) * sizeof(ChunkEntry);

    std::vector<std::byte> tableBytes;
    if (!reader.ReadRange(modelRef_.uri, tableOffset, tableSize, tableBytes))
        return false;
    if (tableBytes.size() != tableSize)
        return false;

    chunks_.resize(header_.chunkCount);
    std::memcpy(chunks_.data(), tableBytes.data(), tableSize);

    return true;
}
}

