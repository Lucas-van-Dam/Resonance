#pragma once
#include <REON/AssetManagement/Asset.h>
#include "Assets/Model/ModelImport.h"
#include <optional>

namespace REON::EDITOR
{
class ImportedSourceStore
{
  public:
    static bool SaveModel(const AssetId& sourceId, const ImportedModel& model);
    static std::optional<ImportedModel> LoadModel(const AssetId& sourceId);
};
} // namespace REON::EDITOR