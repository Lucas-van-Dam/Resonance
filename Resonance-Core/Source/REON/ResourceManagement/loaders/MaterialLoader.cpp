#include "reonpch.h"

#include "MaterialLoader.h"

#include "REON/Rendering/Material.h"

#include "REON/AssetManagement/MaterialBinFormat.h"

#include "REON/Application.h"

namespace REON
{

static AssetId AssetIdFromBytes16(const uint8_t b[16])
{
    AssetId id{};
    std::memcpy(id.bytes.data(), b, 16);
    return id;
}

std::shared_ptr<ResourceBase> MaterialLoader::Load(const AssetKey& key, const ArtifactRef& ref, IBlobReader& reader)
{
    std::vector<std::byte> bytes;
    if (!reader.ReadRange(ref.uri, ref.offset, ref.size, bytes))
        return {};

    if (bytes.size() < sizeof(MatBinHeader))
        return {};

    const MatBinHeader* th = reinterpret_cast<const MatBinHeader*>(bytes.data());
    if (th->magic != MAT_MAGIC || th->version != MAT_VERSION)
        return {};

    auto mat = std::make_shared<Material>();

    std::memcpy(&mat->flatData.albedo.r, &th->baseColorFactor, sizeof(mat->flatData.albedo));
    std::memcpy(&mat->flatData.emissiveFactor.r, &th->emissiveFactor, sizeof(mat->flatData.emissiveFactor));

    mat->setDoubleSided(th->flags & MAT_DOUBLE_SIDED);

    mat->renderingMode = th->flags & MAT_TRANSPARENT ? Transparent : Opaque;
    mat->blendingMode = th->flags & MAT_MODE_BLEND ? Blend : Mask;

    uint32_t flags = 0;

    AssetId TexId = AssetIdFromBytes16(th->baseColorTex);
    if (TexId != NullAssetId)
    {
        flags |= AlbedoTexture;
        mat->albedoTexture = Application::Get().GetEngineServices().resources.GetOrLoad<Texture>(TexId);
        
    }
    TexId = AssetIdFromBytes16(th->mrTex);
    if (TexId != NullAssetId)
    {
        flags |= MetallicRoughnessTexture;
        mat->metallicRoughnessTexture = Application::Get().GetEngineServices().resources.GetOrLoad<Texture>(TexId);
    }
    TexId = AssetIdFromBytes16(th->normalTex);
    if (TexId != NullAssetId)
    {
        flags |= NormalTexture;
        mat->normalTexture = Application::Get().GetEngineServices().resources.GetOrLoad<Texture>(TexId);
    }
    TexId = AssetIdFromBytes16(th->emissiveTex);
    if (TexId != NullAssetId)
    {
        flags |= EmissiveTexture;
        mat->emissiveTexture = Application::Get().GetEngineServices().resources.GetOrLoad<Texture>(TexId);
    }
    TexId = AssetIdFromBytes16(th->specularTex);
    if (TexId != NullAssetId)
    {
        mat->specularTexture = Application::Get().GetEngineServices().resources.GetOrLoad<Texture>(TexId);
    }
    TexId = AssetIdFromBytes16(th->specularColorTex);
    if (TexId != NullAssetId)
    {
        mat->specularColorTexture = Application::Get().GetEngineServices().resources.GetOrLoad<Texture>(TexId);
    }

    mat->flatData.metallic = th->metallic;
    mat->flatData.normalScalar = th->normalScalar;
    mat->flatData.preCompF0 = th->precompF0;
    mat->flatData.roughness = th->roughness;

    mat->materialFlags = flags;

    return mat;
}
} // namespace REON