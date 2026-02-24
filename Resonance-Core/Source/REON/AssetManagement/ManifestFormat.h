#pragma once

#include <cstdint>

namespace REON
{
static constexpr uint32_t MANIFEST_MAGIC = 0x464E414Du;
static constexpr uint16_t MANIFEST_VERSION = 1;

#pragma pack(push, 1)

struct ManifestHeader
{
    uint32_t magic = MANIFEST_MAGIC;
    uint16_t version = MANIFEST_VERSION;
    uint16_t headerSize = sizeof(ManifestHeader);

    uint32_t entryCount;
    uint32_t reserved0;

    uint64_t entriesOffset; // from file start
};

struct AssetKeyBin
{
    uint32_t type; // AssetTypeId
    uint8_t id[16];
};

struct ArtifactRefBin
{
    uint32_t uriOffset; // into string table
    uint32_t uriLength; // bytes (not null-terminated)
    uint64_t offset;
    uint64_t size;
    uint32_t format;
    uint32_t flags;
};

struct ManifestEntry
{
    AssetKeyBin key;
    ArtifactRefBin ref;
};

#pragma pack(pop)
}