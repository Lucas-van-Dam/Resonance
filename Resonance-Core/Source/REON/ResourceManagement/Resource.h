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

template <class T> class ResourceHandle
{
  public:
    AssetKey Key() const noexcept
    {
        return key_;
    }
    std::shared_ptr<T> Lock() const
    {
        return std::static_pointer_cast<T>(ptr_.lock());
    }
    explicit operator bool() const
    {
        return !ptr_.expired();
    }

  private:
    friend class ResourceManager;
    AssetKey key_;
    std::weak_ptr<ResourceBase> ptr_;
};
} // namespace REON