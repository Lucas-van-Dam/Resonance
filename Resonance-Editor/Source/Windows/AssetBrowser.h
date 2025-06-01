#pragma once
#include <vector>
#include <string>
#include <filesystem>
#include <imgui.h>
#include <ProjectManagement/ProjectManager.h>

namespace REON::EDITOR {

	class AssetBrowser
	{
	public:
		void SetRootDirectory(std::filesystem::path directory) {
			m_RootDirectory = directory;
			m_CurrentDirectory = directory;
		}

		void RenderAssetBrowser();

		std::vector<std::string> GetSubdirectories(const std::string& directoryPath);

		std::filesystem::path getSelectedFile() const { return m_SelectedFile; }
		void clearSelectedFile() { m_SelectedFile = ""; m_SelectedDirectory = ""; }

	private:
		std::filesystem::path m_CurrentDirectory;
		std::filesystem::path m_RootDirectory;
		std::filesystem::path m_SelectedDirectory;
		std::filesystem::path m_SelectedFile;

		void RenderDirectoryTree(const fs::path& directory);
	};

}