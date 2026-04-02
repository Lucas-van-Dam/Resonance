#include "MaterialBinWriter.h"

#include "REON/AssetManagement/MaterialBinFormat.h"

namespace REON_EDITOR
{
CookOutput MaterialBinWriter::WriteMaterialBin(const MaterialSourceData& mat, const std::filesystem::path& path)
{
    std::filesystem::create_directories(path.parent_path());

    REON::MatBinHeader header;
    header.headerSize = static_cast<uint16_t>(sizeof(REON::MatBinHeader));
    std::memcpy(header.baseColorFactor, &mat.baseColorFactor.r, sizeof(header.baseColorFactor));
    uint32_t flags = 0;
    flags |= mat.doubleSided ? REON::MAT_DOUBLE_SIDED : 0;
    flags |= mat.renderingMode == REON::Transparent ? REON::MAT_TRANSPARENT : 0;
    flags |= mat.renderingMode == REON::Transparent
                 ? mat.blendingMode == REON::Blend ? REON::MAT_MODE_BLEND : REON::MAT_MODE_MASK
                 : 0;
    flags |= mat.flipNormals ? REON::MAT_FLIP_NORMALS : 0;

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

    REON::ArtifactRef ref{};
    ref.uri = path.filename().generic_string();
    ref.offset = 0;
    ref.size = fileSize;
    ref.flags = REON::ARTIFACT_FLAG_LITTLE_ENDIAN;

    CookOutput output;
    output.artifacts[REON::AssetKey{REON::ASSET_MATERIAL, mat.id}] = ref;
    return output;
}
} // namespace REON::EDITOR