#include "TextureBinWriter.h"

#include "REON/AssetManagement/TextureBinFormat.h"

namespace REON::EDITOR
{

CookOutput TextureBinWriter::WriteTextureBin(const ImportedTexture& texture, const ImportedImage& img,
                                                           const std::filesystem::path& outFile)
{
    std::filesystem::create_directories(outFile.parent_path());

    const uint64_t dataSize = static_cast<uint64_t>(img.rgba8.size());

    TexBinHeader hdr{};
    hdr.headerSize = static_cast<uint16_t>(sizeof(TexBinHeader));
    hdr.width = img.width;
    hdr.height = img.height;
    hdr.mipCount = 1;
    hdr.format = static_cast<uint32_t>(img.srgb ? TexPayloadFormat::RGBA8_SRGB : TexPayloadFormat::RGBA8_UNORM);
    hdr.flags = 0;
    hdr.wrapU = static_cast<uint8_t>(texture.samplerStateWrapU);
    hdr.wrapV = static_cast<uint8_t>(texture.samplerStateWrapV);
    hdr.minFilter = static_cast<uint8_t>(texture.samplerStateMinFilter);
    hdr.magFilter = static_cast<uint8_t>(texture.samplerStateMagFilter);
    hdr.dataOffset = static_cast<uint32_t>(sizeof(TexBinHeader));
    hdr.dataSize = dataSize;

    std::ofstream out(outFile, std::ios::binary | std::ios::trunc);
    if (!out)
        throw std::runtime_error("TextureBinWriter: failed to open output file: " + outFile.string());

    out.write(reinterpret_cast<const char*>(&hdr), sizeof(hdr));
    out.write(reinterpret_cast<const char*>(img.rgba8.data()), static_cast<std::streamsize>(img.rgba8.size()));

    out.flush();
    if (!out)
        throw std::runtime_error("TextureBinWriter: write failed: " + outFile.string());

    const uint64_t fileSize = std::filesystem::file_size(outFile);

    ArtifactRef ref{};
    ref.uri = outFile.filename().generic_string();
    ref.offset = 0;
    ref.size = fileSize;
    ref.flags = ARTIFACT_FLAG_LITTLE_ENDIAN;

    CookOutput output;
    output.artifacts[AssetKey{ASSET_TEXTURE, texture.id}] = ref;
    return output;
}

} // namespace REON::EDITOR
