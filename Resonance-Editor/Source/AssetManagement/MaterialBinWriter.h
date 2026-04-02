#pragma once

#include "AssetManagement/Assets/Material/MaterialSourceData.h"
#include "CookOutput.h"

namespace REON_EDITOR
{
class MaterialBinWriter
{
  public:
    static CookOutput WriteMaterialBin(const MaterialSourceData& mat, const std::filesystem::path& path);
};
}