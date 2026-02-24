#include "CookPipeline.h"

#include "CookOutput.h"
#include "ModelBinWriter.h"
#include "TextureBinWriter.h"
#include "MaterialBinWriter.h"


namespace REON::EDITOR
{
std::unordered_map<AssetTypeId, std::unique_ptr<IImporter>> CookPipeline::m_Importers;

CookPipeline::CookPipeline() : importCtx(cache, AssetRegistry::Instance()) {}

bool CookPipeline::CookAll(const CookOptions& options, BuildQueue& queue)
{
    BuildJob job;
    while (queue.TryDequeue(job))
    {
        if (job.doImport)
        {
            auto it = m_Importers.find(job.type);
            if (it == m_Importers.end())
            {
                REON_ERROR("No importer registered for asset type: {}, skipping", job.type);
                continue;
            }

            const AssetRecord* record = AssetRegistry::Instance().FindById(job.sourceId);
            if (it->second->CanImport(record->sourcePath.extension()))
            {
                ImportResult result = it->second->Import(record->sourcePath, importCtx);
                for (const auto& asset : result.producedAssets)
                {
                    AssetRegistry::Instance().Upsert(asset);
                }
            }
            else
            {
                REON_ERROR("Importer for type {} cannot handle file extension: {}, skipping", job.type,
                           record->sourcePath.extension().string());
            }
        }

        CookOutput cookOutput;

        if (job.type == ASSET_MODEL)
        {
            const auto& model = importCtx.cache.modelCache.at(job.sourceId);
            cookOutput = ModelBinWriter::WriteModelBin(model,
                                                       options.projectRoot / options.cookedRoot /
                                                           (job.sourceId.to_string() + ".modelbin"));

            for (const auto& tex : model.textures)
            {
                auto it = std::find_if(model.images.begin(), model.images.end(),
                                       [&](const ImportedImage& img) { return img.id == tex.imageId; });

                if (it == model.images.end())
                {
                    REON_ERROR("Model {} references texture {}, but image {} not found, skipping texture",
                               model.debugName, tex.debugName, tex.imageId.to_string());
                    continue;
                }

                const auto& output = TextureBinWriter::WriteTextureBin(tex, *it, options.projectRoot / options.cookedRoot /
                                                                          (tex.id.to_string() + ".texbin"));
                cookOutput.assetDeps.push_back(tex.id);
                for (const auto& ref : output.artifacts)
                {
                    cookOutput.artifacts[ref.first] = ref.second;
                }
            }

            for (const auto& mat : model.materials)
            {
                const auto& output = MaterialBinWriter::WriteMaterialBin(mat, options.projectRoot / options.cookedRoot /
                                                                                  (mat.id.to_string() + ".matbin"));

                cookOutput.assetDeps.push_back(mat.id);
                for (const auto& ref : output.artifacts)
                {
                    cookOutput.artifacts[ref.first] = ref.second;
                }
            }
        }

        manifestWriter.Upsert(cookOutput.artifacts);

    }

    manifestWriter.Save(options.projectRoot / options.cookedRoot / "manifest.bin");

    return true;
}
} // namespace REON::EDITOR