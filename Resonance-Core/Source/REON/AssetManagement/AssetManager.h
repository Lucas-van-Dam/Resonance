#pragma once

#include <any>
#include <string>
#include <memory>
#include <unordered_map>

#include "Asset.h"

namespace REON {

	class AssetManager
	{
	public:
		static AssetManager& GetInstance() {
			static AssetManager instance;
			return instance;
		}

		AssetManager(const AssetManager&) = delete;
		AssetManager& operator=(const AssetManager&) = delete;

		template<typename AssetType>
		std::shared_ptr<AssetType> LoadAsset(const std::string& path, std::any metadata = {});

	private:
		AssetManager() = default;
		~AssetManager() = default;

	private:
		std::unordered_map<unsigned int, std::shared_ptr<Asset>> m_Assets;
	};

	template<typename AssetType>
	std::shared_ptr<AssetType> AssetManager::LoadAsset(const std::string& path, std::any metadata)
	{
		unsigned int assetID = std::hash<std::string>{}(path);

		auto it = m_Assets.find(assetID);
		if (it != m_Assets.end()) {
			return std::dynamic_pointer_cast<AssetType>(it->second);
		}

		auto asset = std::make_shared<AssetType>();

		asset->Load(path, metadata);
		//m_Assets[assetID] = asset;

		return asset;
	}

}

