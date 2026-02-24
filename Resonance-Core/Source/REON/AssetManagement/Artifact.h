#pragma once

#include <string>

namespace REON
{
static constexpr uint32_t ARTIFACT_FLAG_LITTLE_ENDIAN = 1u << 0;

struct ArtifactRef
{
    std::string uri;      // relative path or pack URI
    std::uint64_t offset; // byte offset in file/pack
    std::uint64_t size;   // byte length
    std::uint32_t format; // codec/format id, e.g. mesh_v1
    std::uint32_t flags;  // compression, endian, etc.
};
} // namespace REON