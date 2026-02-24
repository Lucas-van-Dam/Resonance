#include "reonpch.h"
#include "ResourceManager.h"
#include <REON/Rendering/Mesh.h>
#include <ReflectionSystem.h>

namespace REON
{
void ResourceManager::SetResolver(std::shared_ptr<IAssetResolver> r) {
    resolver_ = r;
}
void ResourceManager::SetBlobReader(std::shared_ptr<IBlobReader> b) {
    blobReader_ = b;
}
void ResourceManager::RegisterLoader(std::unique_ptr<IResourceLoader> loader) {
    loaders_[loader->Type()] = std::move(loader);
}
} // namespace REON