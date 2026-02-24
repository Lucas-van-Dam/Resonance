#include "reonpch.h"

#include "TextureLoader.h"

#include "REON/AssetManagement/TextureBinFormat.h"
#include "REON/Rendering/Structs/Texture.h"

namespace REON
{

static VkFilter EngineToVkFilter(uint8_t filter)
{
    switch (filter)
    {
    case Nearest:
        return VK_FILTER_NEAREST;
    case Linear:
        return VK_FILTER_LINEAR;
    default:
        return VK_FILTER_LINEAR; // default to linear if unknown
    }
}

static VkSamplerAddressMode EngineToVkSamplerAddressMode(uint8_t mode)
{
    switch (mode)
    {
    case Repeat:
        return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    case MirroredRepeat:
        return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    case ClampToEdge:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    case ClampToBorder:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    default:
        return VK_SAMPLER_ADDRESS_MODE_REPEAT; // default to repeat if unknown
    }
}

std::shared_ptr<ResourceBase> TextureLoader::Load(const AssetKey& key, const ArtifactRef& ref, IBlobReader& reader)
{
    std::vector<std::byte> bytes;
    if (!reader.ReadRange(ref.uri, ref.offset, ref.size, bytes))
        return {};

    if (bytes.size() < sizeof(TexBinHeader))
        return {};

    const TexBinHeader* th = reinterpret_cast<const TexBinHeader*>(bytes.data());
    if (th->magic != 0x42455854u || th->version != 1)
        return {};

    TextureData data;
    data.width = th->width;
    data.height = th->height;
    data.samplerData.magFilter = EngineToVkFilter(th->magFilter);
    data.samplerData.minFilter = EngineToVkFilter(th->minFilter);
    data.samplerData.addressModeU = EngineToVkSamplerAddressMode(th->wrapU);
    data.samplerData.addressModeV = EngineToVkSamplerAddressMode(th->wrapV);
    data.samplerData.addressModeW =
        VK_SAMPLER_ADDRESS_MODE_REPEAT; // default to repeat for 3D (TODO: add proper support later)
    data.pixels.resize(static_cast<size_t>(th->dataSize));

    const std::byte* base = bytes.data();
    if (th->dataOffset + th->dataSize > bytes.size())
        return {};

    std::memcpy(data.pixels.data(), base + th->dataOffset, static_cast<size_t>(th->dataSize));

    return std::make_shared<Texture>(data);
}

} // namespace REON