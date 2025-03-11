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

        // Add an existing resource to the cache
        template <typename ResourceType>
        void AddResource(std::shared_ptr<ResourceType> resource);

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

        //if (metadata.type() == typeid(ResourceType)) {
        //    auto newResource = std::make_shared<ResourceType>(std::any_cast<ResourceType>(metadata));
        //}

        REON_CORE_TRACE("Loading Resource from filePath: {}", filePath);
        // Create a new resource and load it
        auto newResource = std::make_shared<ResourceType>();
        newResource->Load(filePath, metadata);

        // Cache and return the resource using its UID
        resourceCache[newResource->GetID()] = newResource;
        return newResource;
    }

    template<typename ResourceType>
    void ResourceManager::AddResource(std::shared_ptr<ResourceType> resource)
    {
        if (resourceCache.find(resource->GetID()) != resourceCache.end()) {
            REON_CORE_ERROR("Already found resource with UID {}", resource->GetID());
            return;
        }

        resource->Load();

        resourceCache[resource->GetID()] = resource;
    }

}