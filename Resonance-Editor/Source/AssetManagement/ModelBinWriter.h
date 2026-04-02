#pragma once

#include "AssetManagement/Assets/Model/ModelImport.h"
#include "REON/AssetManagement/Artifact.h"
#include "AssetManagement/CookOutput.h"

namespace REON_EDITOR
{
class ModelBinWriter
{
  public:
    static CookOutput WriteModelBin(const ImportedModel& model, const std::filesystem::path& outFile);
};
}