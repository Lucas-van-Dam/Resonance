#pragma once

#include "AssetManagement/Assets/Model/ModelImport.h"
#include "CookOutput.h"

namespace REON::EDITOR
{
class MaterialBinWriter
{
  public:
    static CookOutput WriteMaterialBin(const ImportedMaterial& mat, const std::filesystem::path& path);
};
}