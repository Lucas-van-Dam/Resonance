#pragma once

#include "REON/AssetManagement/Artifact.h"
#include "REON/AssetManagement/Asset.h"
#include "REON/AssetManagement/BlobIO.h"
#include "Resource.h"

namespace REON
{
struct IResourceLoader
{
    virtual ~IResourceLoader() = default;
    virtual AssetTypeId Type() const = 0;
    virtual std::shared_ptr<ResourceBase> Load(const AssetKey& key, const ArtifactRef& ref, IBlobReader& reader) = 0;
};
}