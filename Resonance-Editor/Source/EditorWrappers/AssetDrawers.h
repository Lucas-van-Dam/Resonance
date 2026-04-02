#pragma once
#include <filesystem>
#include <AssetManagement/CookPipeline.h>

namespace REON::EDITOR {

	class AssetDrawers
	{
	public:
        static void DrawInspector_Material(std::filesystem::path path, CookPipeline& pipeline);
		static void DrawInspector_Model(std::filesystem::path path, CookPipeline& pipeline);
	};

}