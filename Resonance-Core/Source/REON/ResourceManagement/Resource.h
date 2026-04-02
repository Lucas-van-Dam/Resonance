#pragma once
#include "REON/AssetManagement/Asset.h"

#include <any>
#include <iostream>
#include <string>

namespace REON
{

struct ResourceBase
{
    virtual ~ResourceBase() = default;
    AssetKey key; // not needed, but can be useful for debugging purposes
    // Possibly add virtual methods for lifecycle management, e.g. Load(), Unload(), etc., current plan is to use purely RAII
};

struct ResourceSlot
{
    AssetKey key;
    std::shared_ptr<ResourceBase> current;
    uint32_t loadedArtifactRevision = 0;
    uint32_t generation = 0;
    mutable std::mutex mutex;
};

template <class T> class ResourceHandle
{
  public:
    AssetKey Key() const noexcept
    {
        return key_;
    }
    std::shared_ptr<T> Lock() const
    {
        auto slot = slot_.lock();
        if (!slot || !slot->current)
            return {};

        return std::static_pointer_cast<T>(slot->current);
    }

    explicit operator bool() const
    {
        auto slot = slot_.lock();
        return slot && slot->current;
    }

  private:
    friend class ResourceManager;
    AssetKey key_;
    std::weak_ptr<ResourceSlot> slot_;
};



} // namespace REON