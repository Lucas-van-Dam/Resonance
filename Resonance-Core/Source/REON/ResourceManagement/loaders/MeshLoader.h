#pragma once

#include "REON/AssetManagement/Asset.h"
#include "REON/AssetManagement/Artifact.h"
#include "REON/ResourceManagement/ResourceLoader.h"
#include "REON/ResourceManagement/Resource.h"
#include "REON/Rendering/Mesh.h"

namespace REON
{
class MeshLoader final : public IResourceLoader
{
  public:
    AssetTypeId Type() const override
    {
        return ASSET_MESH;
    }
    std::shared_ptr<ResourceBase> Load(const AssetKey& key, const ArtifactRef& ref, IBlobReader& reader) override;
};
}