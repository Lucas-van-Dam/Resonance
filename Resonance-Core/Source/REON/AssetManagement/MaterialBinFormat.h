#pragma once

#include <cstdint>

namespace REON
{

    static constexpr uint32_t MakeFourCC(char a, char b, char c, char d)
{
    return (uint32_t(a) << 24) | (uint32_t(b) << 16) | (uint32_t(c) << 8) | uint32_t(d);
}

enum MatFlags : uint32_t
{
    MAT_DOUBLE_SIDED = 1 << 0,
    MAT_TRANSPARENT = 1 << 1,
    MAT_MODE_BLEND = 1 << 2, // Only one of the Blend, Mask modes allowed, and only if MAT_TRANSPARENT
    MAT_MODE_MASK = 1 << 3
};

constexpr uint32_t MAT_MAGIC = MakeFourCC('M', 'A', 'T', 'B');
constexpr uint16_t MAT_VERSION = 1;

struct MatBinHeader
{
    uint32_t magic = MAT_MAGIC;
    uint16_t version = MAT_VERSION;
    uint16_t headerSize;
    uint32_t flags;

    float baseColorFactor[4];
    float metallic;
    float roughness;
    float precompF0;
    float normalScalar;
    float emissiveFactor[4]; // w = alphaCutoff

    uint8_t baseColorTex[16];
    uint8_t normalTex[16];
    uint8_t mrTex[16];
    uint8_t emissiveTex[16];
    uint8_t specularTex[16];
    uint8_t specularColorTex[16];
};
} // namespace REON