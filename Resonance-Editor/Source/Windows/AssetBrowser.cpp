#include "AssetBrowser.h"

namespace REON::EDITOR {

	void AssetBrowser::RenderAssetBrowser() {
		if (m_RootDirectory.empty())
			return;

		ImGui::Begin("Asset Browser");
		if (ImGui::BeginChild("LeftPanel", ImVec2(200, 0), true)) {
			RenderDirectoryTree(m_RootDirectory);
		}
		ImGui::EndChild();

		ImGui::SameLine();
		if (ImGui::BeginChild("RightPanel")) {
			if (m_CurrentDirectory != m_RootDirectory) {
				if (ImGui::Button("..")) {
					m_CurrentDirectory = m_CurrentDirectory.parent_path();
				}
			}

			const float thumbnailSize = 100.0f;  // Size of each item (square)
			const float padding = 10.0f;         // Padding between items
			const float cellSize = thumbnailSize + padding;
			float panelWidth = ImGui::GetContentRegionAvail().x;
			int columns = (int)(panelWidth / cellSize);  // Calculate how many items fit in a row
			if (columns < 1) columns = 1;

			ImGui::Columns(columns, nullptr, false);

			for (const auto& entry : fs::directory_iterator(m_CurrentDirectory)) {
				if (entry.path().extension() == ".meta")
					continue;

				std::string name = entry.path().filename().string();

				ImGui::PushID(name.c_str());
				if (ImGui::Button(name.c_str(), ImVec2(thumbnailSize, thumbnailSize))) {
					if (entry.is_directory()) {
						m_SelectedFile = entry.path();
					}
					else {
						// Handle file selection here
					}
				}

				if (entry.is_directory() && ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
					m_CurrentDirectory = entry.path();  // Navigate into the folder
				}

				if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
					auto path = std::filesystem::relative(entry.path(), m_RootDirectory.parent_path());
					ImGui::SetDragDropPayload("ASSET_BROWSER_ITEM", path.string().c_str(), path.string().size() + 1);
					ImGui::Text("Dragging: %s", name.c_str());
					ImGui::EndDragDropSource();
				}

				// Show the name below the thumbnail
				ImGui::TextWrapped(name.c_str());
				ImGui::NextColumn();
				ImGui::PopID();
			}

			ImGui::Columns(1);
		}
		ImGui::EndChild();
		ImGui::End();
	}

	std::vector<std::string> AssetBrowser::GetSubdirectories(const std::string& directoryPath) {
		std::vector<std::string> subdirectories;

		try {
			for (const auto& entry : std::filesystem::directory_iterator(directoryPath)) {
				if (entry.is_directory()) {
					subdirectories.push_back(entry.path().filename().string());  // Get the folder name only
				}
			}
		}
		catch (const std::filesystem::filesystem_error& e) {
			// Handle any errors, like invalid paths or permissions
			REON_ERROR("Filesystem error while getting subdirectories in assetbrowser");
		}

		return subdirectories;
	}

	void AssetBrowser::RenderDirectoryTree(const fs::path& directory) {
		for (const auto& entry : fs::directory_iterator(directory)) {
			if (entry.is_directory()) {
				ImGuiTreeNodeFlags flags = (entry.path() == m_CurrentDirectory) ? ImGuiTreeNodeFlags_Selected : 0;
				bool open = ImGui::TreeNodeEx(entry.path().filename().string().c_str(), flags | ImGuiTreeNodeFlags_OpenOnArrow);

				if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
					m_CurrentDirectory = entry.path();
				}

				if (open) {
					RenderDirectoryTree(entry.path());  // Recursively render subdirectories
					ImGui::TreePop();
				}
			}
			else if (entry.is_regular_file()) {
				// Show files but make them non-interactive (not clickable)
				ImGui::TextDisabled(entry.path().filename().string().c_str());
			}
		}

	}
}