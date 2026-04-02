#pragma once

#include "REON/AssetManagement/Asset.h"

#include <deque>
#include <unordered_set>

namespace REON_EDITOR
{

enum class BuildReason : uint8_t
{
    MissingCooked,
    SourceChanged,
    SettingsChanged,
    ToolVersionChanged,
    DependencyChanged,
    ForceRebuild
};

struct BuildJob
{
    REON::AssetId sourceId;
    REON::AssetTypeId type;
    BuildReason reason;

    bool doImport = true;
};

class BuildQueue
{
  public:
    void Enqueue(BuildJob item);
    bool TryDequeue(BuildJob& out);

  private:
    std::deque<BuildJob> m_Items;
    std::unordered_set<uint64_t> m_Dedup;
};
} // namespace REON::EDITOR