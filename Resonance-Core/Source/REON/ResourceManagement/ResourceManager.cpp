#include "reonpch.h"
#include "ResourceManager.h"

namespace REON {
	inline void ResourceManager::UnloadResource(std::string uid)
	{
		resourceCache.erase(uid);
	}

	inline void ResourceManager::Clear()
	{
		resourceCache.clear();
	}
}