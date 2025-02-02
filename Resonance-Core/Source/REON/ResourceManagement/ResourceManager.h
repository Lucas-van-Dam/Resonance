#pragma once
#include "Resource.h"

namespace REON {
	class ResourceManager
	{
	public:
        static ResourceManager& GetInstance() {
            static ResourceManager instance;
            return instance;
        }

        ResourceManager(const ResourceManager&) = delete;
        ResourceManager& operator=(const ResourceManager&) = delete;

        // Retrieve a resource by UID (returns nullptr if not found)
        template <typename ResourceType>
        std::shared_ptr<ResourceType> GetResource(std::string uid);

        // Load a resource of a specific type with optional metadata
        template <typename ResourceType>
        std::shared_ptr<ResourceType> LoadResource(const std::string& filePath, std::any metadata = {});

        // Unload a resource by UID
        void UnloadResource(std::string uid);

        // Clear all resources
        void Clear();

    private:
        ResourceManager() = default;
        ~ResourceManager() = default;

    private:
        std::unordered_map<std::string, std::shared_ptr<Resource>> resourceCache;
	};

    template <typename ResourceType>
    std::shared_ptr<ResourceType> ResourceManager::GetResource(std::string uid) {
        auto it = resourceCache.find(uid);
        if (it != resourceCache.end()) {
            return std::dynamic_pointer_cast<ResourceType>(it->second);
        }
        return nullptr;
    }

    template <typename ResourceType>
    std::shared_ptr<ResourceType> ResourceManager::LoadResource(const std::string& filePath, std::any metadata) {
        // Check if the resource is already loaded by its file path
        for (const auto& [uid, resource] : resourceCache) {
            if (resource->GetPath() == filePath) {
                return std::dynamic_pointer_cast<ResourceType>(resource);  // Return the existing resource
            }
        }

        REON_CORE_TRACE("Loading Resource from filePath: {}", filePath);
        // Create a new resource and load it
        auto newResource = std::make_shared<ResourceType>();
        newResource->Load(filePath, metadata);

        // Cache and return the resource using its UID
        resourceCache[newResource->GetID()] = newResource;
        return newResource;
    }

}