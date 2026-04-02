#pragma once

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

  private:
    std::filesystem::path m_projectPath;

};
} // namespace REON::EDITOR