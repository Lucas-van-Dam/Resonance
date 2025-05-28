#include "reonpch.h"
#include "ResourceManager.h"
#include <REON/Rendering/Mesh.h>
#include <ReflectionSystem.h>

namespace REON {
	ResourceManager::ResourceManager() {
		resourceConverters["mesh"] = [](const AssetInfo* info, const std::filesystem::path& path, const std::filesystem::path& basePath) -> std::shared_ptr<ResourceBase> {
			nlohmann::json j;
			std::ifstream file(path);
			if (file.is_open()) {
				file >> j;
				file.close();
			}

			if (j.contains("meshes")) {
				for (auto mesh : j["meshes"]) {
					if (mesh["GUID"] != info->id)
						continue;

					std::shared_ptr<Mesh> meshObj = std::make_shared<Mesh>(info->id);
					meshObj->DeSerialize(mesh);
					return meshObj;
				}

				REON_ERROR("No mesh found in model file");
			}
			return nullptr;
			};
		resourceConverters["material"] = [](const AssetInfo* info, const std::filesystem::path& path, const std::filesystem::path& basePath) {
			nlohmann::json j;
			std::ifstream file(path);
			if (file.is_open()) {
				file >> j;
				file.close();
			}

			std::shared_ptr<Material> material = std::make_shared<Material>();
			material->Deserialize(path, basePath);
			return material;
			};
	}

	ResourceManager::~ResourceManager()
	{
		Destroy();
	}

	inline void ResourceManager::UnloadResource(std::string uid)
	{
		resourceCache.erase(uid);
	}

	inline void ResourceManager::Clear()
	{
		resourceCache.clear();
	}
	void ResourceManager::Destroy()
	{
		resourceConverters.clear();
		for (auto resource : resourceCache) {
			REON_CORE_INFO("Resource: {} has {} references", resource.second->GetName(), resource.second.use_count());
			resource.second.reset();
		}
		Clear();
	}
}