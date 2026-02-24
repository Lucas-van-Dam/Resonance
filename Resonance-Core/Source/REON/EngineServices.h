#pragma once
#include "ResourceManagement/ResourceManager.h"
#include <filesystem>
#include "ResourceManagement/loaders/MeshLoader.h"
#include "ResourceManagement/loaders/TextureLoader.h"
#include "ResourceManagement/loaders/MaterialLoader.h"


namespace REON
{
struct EngineServices
{
    ResourceManager resources;
    IAssetResolver* resolver = nullptr;
    IBlobReader* blobReader = nullptr;

    void Init(const std::filesystem::path& cookedRoot, const std::filesystem::path& manifestPath)
    {
        auto tempResolver = std::make_shared<ManifestAssetResolver>();
        tempResolver->LoadFromFile(manifestPath);

        auto blobs = std::make_shared<DiskBlobReader>(cookedRoot);

        blobReader = blobs.get();
        resolver = tempResolver.get();

        resources.SetResolver(tempResolver);
        resources.SetBlobReader(blobs);
        resources.RegisterLoader(std::make_unique<MaterialLoader>());
        resources.RegisterLoader(std::make_unique<TextureLoader>());
        resources.RegisterLoader(std::make_unique<MeshLoader>());
    }
};
}