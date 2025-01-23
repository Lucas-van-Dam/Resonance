#include "AssetScanner.h"


namespace REON::EDITOR {
	const std::unordered_set<std::string> AssetScanner::supportedExtensions = {
		".gltf", ".png", ".jpg", ".wav", ".mp3" };
}
