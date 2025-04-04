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

	private:
		std::filesystem::path m_CurrentDirectory;
		std::filesystem::path m_RootDirectory;
		std::filesystem::path m_SelectedFile;

		void RenderDirectoryTree(const fs::path& directory);
	};

}