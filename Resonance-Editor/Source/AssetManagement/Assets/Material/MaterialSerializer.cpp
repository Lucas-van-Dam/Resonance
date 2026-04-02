#include "MaterialSerializer.h"

#include <nlohmann/json.hpp>

namespace REON_EDITOR
{

bool MaterialSerializer::Save(const std::filesystem::path& path, const MaterialSourceData& material)
{
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    if (ec)
        return false;

    nlohmann::json j;
    j["id"] = material.id.to_string();
    j["assetType"] = REON::ASSET_MATERIAL;
    j["debugName"] = material.debugName;

    j["baseColorFactor"] = {material.baseColorFactor.x, material.baseColorFactor.y, material.baseColorFactor.z,
                            material.baseColorFactor.w};
    j["metallic"] = material.metallic;
    j["roughness"] = material.roughness;
    j["precompF0"] = material.precompF0;
    j["normalScalar"] = material.normalScalar;
    j["emissiveFactor"] = {material.emissiveFactor.x, material.emissiveFactor.y, material.emissiveFactor.z,
                           material.emissiveFactor.w};
    j["doubleSided"] = material.doubleSided;
    j["flipNormals"] = material.flipNormals;

    j["baseColorTex"] = material.baseColorTex.to_string();
    j["normalTex"] = material.normalTex.to_string();
    j["mrTex"] = material.mrTex.to_string();
    j["emissiveTex"] = material.emissiveTex.to_string();
    j["specularTex"] = material.specularTex.to_string();
    j["specularColorTex"] = material.specularColorTex.to_string();

    j["flags"] = material.flags;
    j["blendingMode"] = static_cast<int>(material.blendingMode);
    j["renderingMode"] = static_cast<int>(material.renderingMode);

    std::ofstream file(path);
    if (!file)
        return false;

    file << j.dump(4);
    return (bool)file;
}

std::optional<MaterialSourceData> MaterialSerializer::Load(const std::filesystem::path& path)
{
    if (!std::filesystem::exists(path))
        return std::nullopt;

    std::ifstream file(path);
    if (!file)
        return std::nullopt;

    nlohmann::json j;
    try
    {
        file >> j;
    }
    catch (...)
    {
        return std::nullopt;
    }

    MaterialSourceData material{};

    try
    {
        material.id = REON::AssetId::from_string(j.at("id").get<std::string>());
        material.debugName = j.value("debugName", "");

        const auto& baseColor = j.at("baseColorFactor");
        if (!baseColor.is_array() || baseColor.size() != 4)
            return std::nullopt;
        material.baseColorFactor = glm::vec4(baseColor[0].get<float>(), baseColor[1].get<float>(),
                                             baseColor[2].get<float>(), baseColor[3].get<float>());

        material.metallic = j.value("metallic", 0.0f);
        material.roughness = j.value("roughness", 1.0f);
        material.precompF0 = j.value("precompF0", 0.04f);
        material.normalScalar = j.value("normalScalar", 1.0f);

        const auto& emissive = j.at("emissiveFactor");
        if (!emissive.is_array() || emissive.size() != 4)
            return std::nullopt;
        material.emissiveFactor = glm::vec4(emissive[0].get<float>(), emissive[1].get<float>(),
                                            emissive[2].get<float>(), emissive[3].get<float>());

        material.doubleSided = j.value("doubleSided", false);
        material.flipNormals = j.value("flipNormals", false);

        material.baseColorTex = REON::AssetId::from_string(j.at("baseColorTex").get<std::string>());
        material.normalTex = REON::AssetId::from_string(j.at("normalTex").get<std::string>());
        material.mrTex = REON::AssetId::from_string(j.at("mrTex").get<std::string>());
        material.emissiveTex = REON::AssetId::from_string(j.at("emissiveTex").get<std::string>());
        material.specularTex = REON::AssetId::from_string(j.at("specularTex").get<std::string>());
        material.specularColorTex = REON::AssetId::from_string(j.at("specularColorTex").get<std::string>());

        material.flags = j.value("flags", 0u);
        material.blendingMode =
            static_cast<REON::BlendingModes>(j.value("blendingMode", static_cast<int>(REON::BlendingModes::Mask)));
        material.renderingMode =
            static_cast<REON::RenderingModes>(j.value("renderingMode", static_cast<int>(REON::RenderingModes::Opaque)));
    }
    catch (...)
    {
        return std::nullopt;
    }

    return material;
}

} // namespace REON::EDITOR
