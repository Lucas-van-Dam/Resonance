#include "MaterialBinWriter.h"

#include "REON/AssetManagement/MaterialBinFormat.h"

namespace REON::EDITOR
{
CookOutput MaterialBinWriter::WriteMaterialBin(const ImportedMaterial& mat, const std::filesystem::path& path)
{
    std::filesystem::create_directories(path.parent_path());

    MatBinHeader header;
    header.headerSize = static_cast<uint16_t>(sizeof(MatBinHeader));
    std::memcpy(header.baseColorFactor, &mat.baseColorFactor.r, sizeof(header.baseColorFactor));
    uint32_t flags = 0;
    flags |= mat.doubleSided ? MAT_DOUBLE_SIDED : 0;
    flags |= mat.renderingMode == Transparent ? MAT_TRANSPARENT : 0;
    flags |= mat.renderingMode == Transparent ? mat.blendingMode == Blend ? MAT_MODE_BLEND : MAT_MODE_MASK : 0;

    header.flags = flags;

    std::memcpy(header.emissiveFactor, &mat.emissiveFactor.r, sizeof(header.emissiveFactor));

    std::copy(mat.baseColorTex.begin(), mat.baseColorTex.end(), header.baseColorTex);
    std::copy(mat.mrTex.begin(), mat.mrTex.end(), header.mrTex);
    std::copy(mat.normalTex.begin(), mat.normalTex.end(), header.normalTex);
    std::copy(mat.emissiveTex.begin(), mat.emissiveTex.end(), header.emissiveTex);
    std::copy(mat.specularTex.begin(), mat.specularTex.end(), header.specularTex);
    std::copy(mat.specularColorTex.begin(), mat.specularColorTex.end(), header.specularColorTex);

    header.metallic = mat.metallic;
    header.normalScalar = mat.normalScalar;
    header.precompF0 = mat.precompF0;
    header.roughness = mat.roughness;

    std::ofstream out(path, std::ios::binary | std::ios::trunc);

    out.write(reinterpret_cast<const char*>(&header), sizeof(header));

    out.flush();

    const uint64_t fileSize = std::filesystem::file_size(path);

    ArtifactRef ref{};
    ref.uri = path.filename().generic_string();
    ref.offset = 0;
    ref.size = fileSize;
    ref.flags = ARTIFACT_FLAG_LITTLE_ENDIAN;

    CookOutput output;
    output.artifacts[AssetKey{ASSET_MATERIAL, mat.id}] = ref;
    return output;
}
} // namespace REON::EDITOR