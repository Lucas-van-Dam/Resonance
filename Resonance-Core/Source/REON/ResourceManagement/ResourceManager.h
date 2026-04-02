#pragma once

#include "REON/AssetManagement/Asset.h"
#include "REON/AssetManagement/AssetResolver.h"
#include "REON/AssetManagement/BlobIO.h"
#include "REON/Logger.h"
#include "Resource.h"
#include "ResourceLoader.h"

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

    void NotifyResourceChanged(const AssetKey&, const ArtifactRef&);

    template <class T> ResourceHandle<T> GetOrLoad(const AssetId& id);

    void Evict(const AssetKey& key); // optional
    void Clear();                    // optional
  private:

    bool ReloadSlot(const AssetKey& key, const std::shared_ptr<ResourceSlot>& slot, const ArtifactRef& ref);

    std::shared_ptr<IAssetResolver> resolver_;
    std::shared_ptr<IBlobReader> blobReader_;
    std::unordered_map<AssetTypeId, std::unique_ptr<IResourceLoader>> loaders_;
    std::unordered_map<AssetKey, std::shared_ptr<ResourceSlot>, AssetKeyHash> cache_;
    mutable std::mutex cacheMutex_; // if thread-safe
};

template <class T> inline ResourceHandle<T> ResourceManager::GetOrLoad(const AssetId& id)
{
    AssetKey key{T::kType, id};
    std::shared_ptr<ResourceSlot> slot;

    {
        std::scoped_lock lk(cacheMutex_);
        auto [it, inserted] = cache_.try_emplace(key, nullptr);
        if (inserted || !it->second)
        {
            it->second = std::make_shared<ResourceSlot>();
            it->second->key = key;
        }
        slot = it->second;
    }

    ArtifactRef ref{};
    if (!resolver_ || !resolver_->Resolve(key, ref))
    {
        REON_WARN("Failed to resolve: {}", key.type);
        return {};
    }

    {
        std::scoped_lock lk(slot->mutex);
        if (slot->current && slot->loadedArtifactRevision == ref.revision)
        {
            ResourceHandle<T> h;
            h.key_ = key;
            h.slot_ = slot;
            return h;
        }
    }

    auto lit = loaders_.find(key.type);
    if (lit == loaders_.end())
        return {};

    auto res = lit->second->Load(key, ref, *blobReader_);
    if (!res)
        return {};

    {
        std::scoped_lock lk(slot->mutex);
        if (!slot->current || slot->loadedArtifactRevision != ref.revision)
        {
            auto old = slot->current;
            slot->current = res;
            slot->loadedArtifactRevision = ref.revision;
            ++slot->generation;

            // publish AssetReloaded(key, slot->generation) here if old existed
        }
    }

    ResourceHandle<T> h;
    h.key_ = key;
    h.slot_ = slot;
    return h;
}

} // namespace REON