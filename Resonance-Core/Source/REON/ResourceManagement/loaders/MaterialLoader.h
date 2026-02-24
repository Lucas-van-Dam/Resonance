#pragma once
#include "REON/ResourceManagement/ResourceLoader.h"

namespace REON
{
class MaterialLoader : public IResourceLoader
{
	public:
	AssetTypeId Type() const override
	{
		return ASSET_MATERIAL;
	}
    std::shared_ptr<ResourceBase> Load(const AssetKey& key, const ArtifactRef& ref, IBlobReader& reader) override;
};
}