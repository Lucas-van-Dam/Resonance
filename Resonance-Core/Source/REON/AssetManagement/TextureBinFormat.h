#pragma once

#include <cstdint>

namespace REON
{
enum class TexPayloadFormat : uint32_t
{
    RGBA8_UNORM = 1,
    RGBA8_SRGB = 2,
};

struct TexBinHeader
{
    uint32_t magic = 0x42455854u; // 'TEXB'
    uint16_t version = 1;    // 1
    uint16_t headerSize; // sizeof(TexBinHeader)

    uint32_t width;
    uint32_t height;
    uint32_t mipCount;

    uint32_t format; // TexFormat
    uint32_t flags;  // compression, etc.

    uint8_t wrapU;
    uint8_t wrapV;
    uint8_t minFilter;
    uint8_t magFilter;

    uint32_t dataOffset; // where mip0 starts
    uint64_t dataSize;   // total payload bytes
};
}