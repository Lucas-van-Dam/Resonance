#include "AssetScanner.h"


namespace REON::EDITOR {
	const std::unordered_set<std::string> AssetScanner::primaryAssetExtensions = { ".model", ".material" };
	const std::unordered_set<std::string> AssetScanner::supportedExtensions = {
		".gltf", ".glb", ".png", ".jpg", ".wav", ".mp3", ".model", ".material", ".img"};
}
