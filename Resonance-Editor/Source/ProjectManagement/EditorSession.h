#pragma once

#include "AssetManagement/BuildQueue.h"
#include "AssetManagement/CookPipeline.h"
#include "REON/GameHierarchy/GameObject.h"
#include "Windows/AssetBrowser.h"

#include <filesystem>

namespace REON_EDITOR
{
class EditorSession
{
  public:
    explicit EditorSession(const std::filesystem::path& projectPath);
    ~EditorSession() = default;

    void OnUpdate();
    void OnRender();

    const std::filesystem::path& GetProjectPath() const
    {
        return m_projectPath;
    }

    std::shared_ptr<REON::GameObject> m_SelectedObject;

    AssetBrowser m_AssetBrowser;

    CookPipeline cookPipeline;

    BuildQueue m_BuildQueue;

  private:
    std::filesystem::path m_projectPath;
};
} // namespace REON_EDITOR