#pragma once
#include <cstdint>
#include <cstring>

static constexpr uint32_t MakeFourCC(char a, char b, char c, char d)
{
    return (uint32_t(a) << 24) | (uint32_t(b) << 16) | (uint32_t(c) << 8) | uint32_t(d);
}

enum class ChunkType : uint32_t
{
    SCENE = 1,
    MESH_INDEX = 2,
    MESH_DATA = 3,
    SKIN_DATA = 4,
    RIG = 5,
};

constexpr uint32_t FILE_MAGIC = MakeFourCC('R', 'E', 'O', 'N');
constexpr uint16_t FILE_VERSION = 1;

enum FileFlags : uint32_t
{
    HAS_RIG = 1 << 0,
};

struct FileHeader
{
    uint32_t magic = FILE_MAGIC;
    uint16_t version = FILE_VERSION;
    uint16_t endian = 1; // 1 = little
    uint32_t chunkCount = 0;
    uint32_t flags = 0;
    uint8_t rigId[16]; // valid if flags & HAS_RIG
    uint64_t fileBytes = 0;
    //uint8_t reserved0[20]; 
};

struct ChunkEntry
{
    ChunkType type;
    uint32_t flags;
    uint64_t offset;
    uint64_t size;
};

constexpr uint32_t MESH_MAGIC = MakeFourCC('M', 'E', 'S', 'H');

struct MeshHeader
{
    uint32_t magic = MESH_MAGIC;
    uint32_t version = 1;
    uint32_t flags = 0;
    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;

    uint32_t posOffset = 0;
    uint32_t nrmOffset = 0;
    uint32_t tanOffset = 0;
    uint32_t uv0Offset = 0;
    uint32_t idxOffset = 0;
    uint32_t colorOffset = 0;
    uint32_t joint0offset = 0;
    uint32_t joint1offset = 0;
    uint32_t weight0offset = 0;
    uint32_t weight1offset = 0;

    uint32_t subMeshOffset = 0; // byte offset from start of mesh blob
    uint32_t subMeshCount = 0;
};

struct SubMeshEntry
{
    uint32_t indexOffset; // in indices (not bytes)
    uint32_t indexCount;
    uint32_t materialId; // slot index (0,1,2)
    uint32_t reserved;
};

struct MeshIndexEntry
{
    uint8_t id[16];
    uint32_t version = 1;

    uint64_t dataOffset;
    uint64_t dataSize; // bytes

    uint32_t vertexCount;
    uint32_t indexCount;
    uint32_t flags;
    uint32_t reserved;
};

struct SkinRecord
{
    uint32_t skeletonNode; // node index (UINT32_MAX if none)
    uint32_t jointCount;

    uint32_t jointsOffset; // uint32_t[jointCount]
    uint32_t ibmOffset;    // glm::mat4[jointCount]
};

struct RigChunkHeader
{
    uint32_t version = 1;
    uint8_t rigId[16];

    uint32_t jointCount;
    uint32_t skinCount;

    uint32_t jointNodeIdsOffset;

    uint32_t skinOffset; // uint32_t[skinIndexCount]
};

struct SceneNode
{
    uint32_t parent;      // UINT32_MAX if none
    uint32_t firstChild;  // UINT32_MAX if none
    uint32_t nextSibling; // UINT32_MAX if none
    uint32_t version = 1;

    char debugName[128];

    uint8_t nodeId[16];   // UINT32_MAX if none

    float t[3];
    float r[4];
    float s[3];

    uint32_t flags;
    uint32_t reserved0;

    uint32_t skinIndex; 

    uint8_t meshId[16];     // zero if none
    uint32_t materialCount;
    uint8_t materialId[160]; // max of 10 materials per node
};
static_assert(std::is_trivially_copyable_v<SceneNode>);
static_assert(alignof(SceneNode) == 4);
static_assert(sizeof(SceneNode) == 392);