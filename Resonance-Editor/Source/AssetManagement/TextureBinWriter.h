#pragma once

#include "AssetManagement/Assets/Model/ModelImport.h"
#include "CookOutput.h"

namespace REON::EDITOR
{
class TextureBinWriter
{
  public:
    static CookOutput WriteTextureBin(const ImportedTexture& texture, const ImportedImage& img, const std::filesystem::path& outFile);
};
} // namespace REON::EDITOR