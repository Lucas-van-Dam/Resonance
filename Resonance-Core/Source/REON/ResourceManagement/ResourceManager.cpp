#include "reonpch.h"

#include "ResourceManager.h"

#include <REON/Rendering/Mesh.h>
#include <ReflectionSystem.h>

namespace REON
{
void ResourceManager::SetResolver(std::shared_ptr<IAssetResolver> r)
{
    resolver_ = r;
}
void ResourceManager::SetBlobReader(std::shared_ptr<IBlobReader> b)
{
    blobReader_ = b;
}
void ResourceManager::RegisterLoader(std::unique_ptr<IResourceLoader> loader)
{
    loaders_[loader->Type()] = std::move(loader);
}

void ResourceManager::NotifyResourceChanged(const AssetKey& key, const ArtifactRef& ref)
{
    std::shared_ptr<ResourceSlot> slot;

    {
        std::scoped_lock lk(cacheMutex_);
        auto it = cache_.find(key);
        if (it == cache_.end())
            return;

        slot = it->second;
    }

    bool reloadNeeded = false;

    {
        std::scoped_lock lk(slot->mutex);
        if (ref.revision != slot->loadedArtifactRevision)
        {
            reloadNeeded = true;
        }
    }

    if (reloadNeeded)
        ReloadSlot(key, slot, ref);
}

bool ResourceManager::ReloadSlot(const AssetKey& key, const std::shared_ptr<ResourceSlot>& slot, const ArtifactRef& ref)
{
    auto lit = loaders_.find(key.type);
    if (lit == loaders_.end())
        return false;

    auto res = lit->second->Load(key, ref, *blobReader_);
    if (!res)
        return false;

    std::shared_ptr<ResourceBase> old;
    uint32_t generation = 0;
    bool swapped = false;

    {
        std::scoped_lock lk(slot->mutex);

        // another thread may already have refreshed it
        if (slot->loadedArtifactRevision == ref.revision && slot->current)
            return true;

        old = slot->current;
        slot->current = res;
        slot->loadedArtifactRevision = ref.revision;
        generation = ++slot->generation;
        swapped = true;
    }

    return true;
}

} // namespace REON