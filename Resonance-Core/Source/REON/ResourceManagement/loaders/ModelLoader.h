#pragma once
#include "REON/AssetManagement/Artifact.h"
#include "REON/AssetManagement/Asset.h"
#include "REON/GameHierarchy/Scene.h"

namespace REON
{
class ModelLoader
{
  public:
    static bool LoadModelFromFile(AssetId id,  std::shared_ptr<Scene> scene);
};
}