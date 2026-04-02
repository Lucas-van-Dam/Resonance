#include "AssetScanner.h"


namespace REON_EDITOR {
	const std::unordered_set<std::string> AssetScanner::primaryAssetExtensions = {  ".mat" };
	const std::unordered_set<std::string> AssetScanner::supportedExtensions = {
		".gltf", ".glb", ".png", ".jpg", ".wav", ".mp3", ".material"};
}
