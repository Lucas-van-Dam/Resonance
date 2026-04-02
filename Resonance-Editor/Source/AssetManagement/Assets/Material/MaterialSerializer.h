#pragma once

#include "MaterialSourceData.h"

#include <filesystem>
#include <optional>

namespace REON::EDITOR
{
class MaterialSerializer
{
  public:
    static bool Save(const std::filesystem::path& path, const MaterialSourceData& mat);
    static std::optional<MaterialSourceData> Load(const std::filesystem::path& path);
};
} // namespace REON::EDITOR