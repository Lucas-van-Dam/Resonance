#pragma once
#include <filesystem>

namespace REON::EDITOR {

	class AssetDrawers
	{
	public:
		static void DrawInspector_Material(std::filesystem::path path);
		static void DrawInspector_Model(std::filesystem::path path);
	};

}