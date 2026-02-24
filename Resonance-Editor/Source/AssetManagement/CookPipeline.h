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
    bool CookAll(const CookOptions&, BuildQueue&);
    bool CookDirty(const CookOptions&);

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
};
} // namespace REON::EDITOR