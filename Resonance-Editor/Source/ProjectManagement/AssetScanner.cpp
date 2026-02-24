#include "AssetScanner.h"


namespace REON::EDITOR {
	const std::unordered_set<std::string> AssetScanner::primaryAssetExtensions = {  ".material" };
	const std::unordered_set<std::string> AssetScanner::supportedExtensions = {
		".gltf", ".glb", ".png", ".jpg", ".wav", ".mp3", ".material"};
}
