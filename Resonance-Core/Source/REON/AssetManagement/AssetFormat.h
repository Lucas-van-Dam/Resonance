#pragma once

#include <cstdint>

namespace REON
{
enum class ChunkType : std::uint32_t
{
    Scene = 1,
    MeshIndex = 2,
    MeshData = 3,
    MaterialIndex = 4,
    MaterialData = 5,
    TextureIndex = 6,
    TextureData = 7,
    DebugStrings = 100,
};

#pragma pack(push, 1)
struct FileHeader
{
    std::uint32_t magic;   // 'RMDL'
    std::uint16_t version; // 1
    std::uint16_t endian;  // 1 = little
    std::uint32_t chunkCount;
    std::uint32_t reserved;
    std::uint64_t chunkTableOffset;
    std::uint64_t fileSize;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct ChunkEntry
{
    ChunkType type;
    std::uint32_t flags;
    std::uint64_t offset;
    std::uint64_t size;
};
#pragma pack(pop)


}