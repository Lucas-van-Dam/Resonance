#pragma once
#include "Resource.h"
#include "REON/AssetManagement/AssetRegistry.h"
#include <typeindex>

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
        void AddResource(std::shared_ptr<ResourceType> resource, std::any metadata = {});

        template<typename T>
        std::string GetTypeName() const;

        // Unload a resource by UID
        void UnloadResource(std::string uid);

        // Clear all resources
        void Clear();

        void Destroy();

        std::shared_ptr<ResourceBase> GetResourceFromAsset(AssetInfo* info, const std::filesystem::path& projectPath) {
            auto it = resourceConverters.find(info->type);
            if (it != resourceConverters.end()) {
                return it->second(info, projectPath.string() + "\\" + info->path.string(), projectPath);
            }
            REON_WARN("No resource converter for type {}", info->type);
            return nullptr;
        }

    private:
        ResourceManager();
        ~ResourceManager();

    private:
        std::unordered_map<std::string, std::shared_ptr<ResourceBase>> resourceCache;
        //std::unordered_map<std::string, std::vector<std::shared_ptr<ResourceBase>>> resourcesByType;
        std::unordered_map<std::string, std::function<std::shared_ptr<ResourceBase>(const AssetInfo* info, const std::filesystem::path& path, const std::filesystem::path& basePath)>> resourceConverters;
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

        //std::string typeName = GetTypeName<ResourceType>();
        //resourcesByType[typeName].push_back(newResource);

        newResource->Load(filePath, metadata);

        // Cache and return the resource using its UID
        resourceCache[newResource->GetID()] = newResource;
        return newResource;
    }

    template<typename ResourceType>
    void ResourceManager::AddResource(std::shared_ptr<ResourceType> resource, std::any metadata)
    {
        if (resourceCache.find(resource->GetID()) != resourceCache.end()) {
            REON_CORE_ERROR("Already found resource with UID {}", resource->GetID());
            return;
        }

        //std::string typeName = GetTypeName<ResourceType>();
        //resourcesByType[typeName].push_back(resource);

        resource->Load();

        resourceCache[resource->GetID()] = resource;
    }

    template<typename T>
    std::string ResourceManager::GetTypeName() const
    {
        std::string mangledName = typeid(T).name();
        size_t pos = mangledName.find_last_of("::");
        if (pos != std::string::npos) {
            return mangledName.substr(pos + 1); // Skip "::"
        }
        return mangledName;
    }

}