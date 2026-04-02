#include "CookPipeline.h"

#include "Assets/Material/MaterialSerializer.h"
#include "Assets/Material/MaterialSourceData.h"
#include "CookOutput.h"
#include "ImportedSourceStore.h"
#include "MaterialBinWriter.h"
#include "ModelBinWriter.h"
#include "TextureBinWriter.h"

namespace REON::EDITOR
{
std::unordered_map<AssetTypeId, std::unique_ptr<IImporter>> CookPipeline::m_Importers;

CookPipeline::CookPipeline() : importCtx(cache, AssetRegistry::Instance()) {}

void CookPipeline::SetOptions(CookOptions options)
{
    this->options = options;
}

bool CookPipeline::CookAll(BuildQueue& queue)
{
    BuildJob job;
    while (queue.TryDequeue(job))
    {
        const AssetRecord* record = AssetRegistry::Instance().FindById(job.sourceId);
        if (!record)
            continue;

        if (job.doImport)
        {
            if (!runImport(job, *record, queue))
                continue;
        }

        CookOutput cookOutput;

        cookAsset(job, *record, cookOutput);

        manifestWriter.Upsert(cookOutput.artifacts);
    }

    manifestWriter.Save(options.projectRoot / options.cookedRoot / "manifest.bin");

    return true;
}

bool CookPipeline::runImport(const BuildJob& job, const AssetRecord& record, BuildQueue& queue)
{
    auto it = m_Importers.find(job.type);
    if (it == m_Importers.end())
    {
        REON_ERROR("No importer registered for asset type: {}, skipping", job.type);
        return false;
    }

    if (record.sourcePath.empty())
    {
        REON_ERROR("Asset {} has no source path for import", job.sourceId.to_string());
        return false;
    }

    ImportResult result = it->second->Import(options.projectRoot / record.sourcePath);

    for (const auto& produced : result.producedAssets)
    {
        AssetRegistry::Instance().Upsert(produced);

        queue.Enqueue(BuildJob{.sourceId = produced.id, .type = produced.type, .doImport = false});
    }

    return true;
}

bool CookPipeline::cookAsset(const BuildJob& job, const AssetRecord& record, CookOutput& out)
{
    switch (record.type)
    {
    case ASSET_MODEL:
        out = CookModel(record);
        return true;

    case ASSET_MATERIAL:
        out = CookMaterial(record);
        return true;

    case ASSET_TEXTURE:
        out = CookTexture(record);
        return true;

    default:
        return false;
    }
}

CookOutput CookPipeline::CookModel(const AssetRecord& record)
{
    auto importedModel = ImportedSourceStore::LoadModel(record.id);

    if (!importedModel.has_value())
    {
        REON_ERROR("CookModel: Failed to load imported model for asset {}, cannot cook", record.id.to_string());
        return {};
    }

    CookOutput output = ModelBinWriter::WriteModelBin(importedModel.value(), options.projectRoot / options.cookedRoot /
                                                                                 (record.id.to_string() + ".modelbin"));

    return output;
}

// TODO: make texture also have sourceData instead of just from model, currently out of scope
CookOutput CookPipeline::CookTexture(const AssetRecord& record)
{
    if (record.origin != AssetOrigin::ImportedSubAsset)
    {
        REON_ERROR("CookTexture: Only imported textures as part of models are available for now, asset {} is not an imported sub-asset", record.id.to_string());
        return {};
    }

    auto importedModel = ImportedSourceStore::LoadModel(record.parentSourceId);

            if (!importedModel.has_value())
    {
        REON_ERROR("CookMaterial: Failed to load imported model for asset {}, cannot cook", record.id.to_string());
        return {};
    }
    const auto& textures = importedModel.value().textures;
    auto texture =
        std::find_if(textures.begin(), textures.end(), [&](const ImportedTexture& m) { return m.id == record.id; });
    if (texture == textures.end())
    {
        REON_ERROR("CookMaterial: Failed to find material with id {} in imported model {}, cannot cook",
                   record.id.to_string(), importedModel.value().debugName);
        return {};
    }

    auto it = std::find_if(importedModel.value().images.begin(), importedModel.value().images.end(),
                           [&](const ImportedImage& img) { return img.id == texture->imageId; });

    if (it == importedModel.value().images.end())
    {
        REON_ERROR("texture {} references image {}, but image {} not found, skipping texture", texture->debugName,
                   texture->imageId.to_string(), texture->imageId.to_string());
        return {};
    }

    CookOutput output = TextureBinWriter::WriteTextureBin(
        *texture, *it, options.projectRoot / options.cookedRoot / (texture->id.to_string() + ".texbin"));

    return output;
}

CookOutput CookPipeline::CookMaterial(const AssetRecord& record)
{
    MaterialSourceData source;

    if (record.origin == AssetOrigin::Native)
    {
        source = MaterialSerializer::Load(options.projectRoot / record.sourcePath).value();
    }
    else if (record.origin == AssetOrigin::ImportedSubAsset)
    {
        auto importedModel = ImportedSourceStore::LoadModel(record.parentSourceId);

        if (!importedModel.has_value())
        {
            REON_ERROR("CookMaterial: Failed to load imported model for asset {}, cannot cook", record.id.to_string());
            return {};
        }
        const auto& mats = importedModel.value().materials;
        auto it = std::find_if(mats.begin(), mats.end(), [&](const ImportedMaterial& m) { return m.id == record.id; });
        if (it == mats.end())
        {
            REON_ERROR("CookMaterial: Failed to find material with id {} in imported model {}, cannot cook",
                       record.id.to_string(), importedModel.value().debugName);
            return {};
        }

        const ImportedMaterial& mat = *it;
        source = FromImportedMaterial(mat);
    }

    CookOutput output = MaterialBinWriter::WriteMaterialBin(source, options.projectRoot / options.cookedRoot /
                                                                      (source.id.to_string() + ".matbin"));

    return output;
}
} // namespace REON::EDITOR