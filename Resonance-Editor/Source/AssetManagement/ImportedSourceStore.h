#pragma once
#include <REON/AssetManagement/Asset.h>
#include "Assets/Model/ModelImport.h"
#include <optional>

namespace REON_EDITOR
{
class ImportedSourceStore
{
  public:
    static bool SaveModel(const REON::AssetId& sourceId, const ImportedModel& model);
    static std::optional<ImportedModel> LoadModel(const REON::AssetId& sourceId);
};
} // namespace REON::EDITOR