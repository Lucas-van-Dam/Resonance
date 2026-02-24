#pragma once

#include "REON/AssetManagement/Asset.h"
#include "REON/AssetManagement/AssetResolver.h"
#include "REON/AssetManagement/BlobIO.h"
#include "Resource.h"
#include "ResourceLoader.h"

#include "REON/Logger.h"

#include <memory>
#include <unordered_map>

namespace REON
{
class ResourceManager
{
  public:
    void SetResolver(std::shared_ptr<IAssetResolver> r);
    void SetBlobReader(std::shared_ptr<IBlobReader> b);
    void RegisterLoader(std::unique_ptr<IResourceLoader> loader);

    template <class T> ResourceHandle<T> GetOrLoad(const AssetId& id);

    void Evict(const AssetKey& key); // optional
    void Clear();                    // optional
  private:
    std::shared_ptr<IAssetResolver> resolver_;
    std::shared_ptr<IBlobReader> blobReader_;
    std::unordered_map<AssetTypeId, std::unique_ptr<IResourceLoader>> loaders_;
    std::unordered_map<AssetKey, std::shared_ptr<ResourceBase>, AssetKeyHash> cache_;
    mutable std::mutex cacheMutex_; // if thread-safe
};

template <class T> inline ResourceHandle<T> ResourceManager::GetOrLoad(const AssetId& id)
{
    AssetKey key{T::kType, id};

    { // cache check
        std::scoped_lock lk(cacheMutex_);
        auto it = cache_.find(key);
        if (it != cache_.end())
        {
            ResourceHandle<T> h;
            h.key_ = key;
            h.ptr_ = it->second;
            return h;
        }
    }

    ArtifactRef ref{};
    if (!resolver_ || !resolver_->Resolve(key, ref))
    {
        REON_WARN("Failed to resolve: {}", key.type);
        return {};
    }

    auto lit = loaders_.find(key.type);
    if (lit == loaders_.end())
        return {};

    auto res = lit->second->Load(key, ref, *blobReader_);
    if (!res)
        return {};

    { // insert
        std::scoped_lock lk(cacheMutex_);
        cache_[key] = res;
    }

    ResourceHandle<T> h;
    h.key_ = key;
    h.ptr_ = res;
    return h;
}

} // namespace REON