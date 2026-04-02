#pragma once

#include "BuildQueue.h"

#include <filesystem>
#include "AssetImporter.h"

#include "ManifestWriter.h"

namespace REON::EDITOR
{
struct CookOptions
{
    std::filesystem::path projectRoot;
    std::filesystem::path cookedRoot;
    bool embedDebugChunks = true;
};

class CookPipeline
{
  public:
    CookPipeline();
    void SetOptions(CookOptions options);
    bool CookAll(BuildQueue&);
    bool CookDirty(const CookOptions&);
    bool runImport(const BuildJob& job, const AssetRecord& record, BuildQueue& queue);

    static void RegisterImporter(AssetTypeId type, std::unique_ptr<IImporter> importer)
    {
        m_Importers[type] = std::move(importer);
    }

  private:
      //importer map
    static std::unordered_map<AssetTypeId, std::unique_ptr<IImporter>> m_Importers;
    ImportCache cache;
    ImportContext importCtx;
    ManifestWriter manifestWriter;
    CookOptions options;
};
} // namespace REON::EDITOR