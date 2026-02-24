#include "nlohmann/json.hpp"
#include "ProjectManagement/MetadataGenerator.h"


namespace REON::EDITOR
{
static void InitModelMeta(nlohmann::json& meta)
{
    meta["importSettings"] = {{"scale", 1.0}, {"generateTangents", true}};

    meta["stableIds"] = nlohmann::json::object();
    meta["outputs"] = {{"meshes", nlohmann::json::array()},
                       {"materials", nlohmann::json::array()},
                       {"textures", nlohmann::json::array()}};

    meta["build"] = {{"lastBuildKey", 0}, {"cookedArtifact", ""}};
}

// Registration (could be in module init)
static bool registered = []
{
    MetadataGenerator::Register(".glb", {ASSET_MODEL, &InitModelMeta});
    MetadataGenerator::Register(".gltf", {ASSET_MODEL, &InitModelMeta});
    return true;
}();

} // namespace REON::EDITOR