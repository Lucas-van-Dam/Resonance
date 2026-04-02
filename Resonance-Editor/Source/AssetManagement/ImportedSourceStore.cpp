#include "ImportedSourceStore.h"

#include <filesystem>
#include <fstream>
#include <ProjectManagement/ProjectManager.h>

namespace fs = std::filesystem;

namespace REON::EDITOR
{

namespace
{

constexpr std::uint32_t kMagic = 0x494D444C; // IMDL
constexpr std::uint32_t kVersion = 1;

template <typename T> bool WriteRaw(std::ostream& os, const T& v)
{
    os.write(reinterpret_cast<const char*>(&v), sizeof(T));
    return (bool)os;
}

template <typename T> bool ReadRaw(std::istream& is, T& v)
{
    is.read(reinterpret_cast<char*>(&v), sizeof(T));
    return (bool)is;
}

bool WriteString(std::ostream& os, const std::string& s)
{
    std::uint64_t len = s.size();
    if (!WriteRaw(os, len))
        return false;

    if (len > 0)
    {
        os.write(s.data(), (std::streamsize)len);
        if (!os)
            return false;
    }
    return true;
}

bool ReadString(std::istream& is, std::string& s)
{
    std::uint64_t len = 0;
    if (!ReadRaw(is, len))
        return false;

    s.resize((size_t)len);
    if (len > 0)
    {
        is.read(s.data(), (std::streamsize)len);
        if (!is)
            return false;
    }
    return true;
}

bool WriteAssetId(std::ostream& os, const AssetId& id)
{
    os.write(reinterpret_cast<const char*>(id.bytes.data()), 16);
    return (bool)os;
}

bool ReadAssetId(std::istream& is, AssetId& id)
{
    is.read(reinterpret_cast<char*>(id.bytes.data()), 16);
    return (bool)is;
}

bool WriteAssetIdVector(std::ostream& os, const std::vector<AssetId>& v)
{
    std::uint64_t count = v.size();
    if (!WriteRaw(os, count))
        return false;

    for (const auto& id : v)
        if (!WriteAssetId(os, id))
            return false;

    return true;
}

bool ReadAssetIdVector(std::istream& is, std::vector<AssetId>& v)
{
    std::uint64_t count = 0;
    if (!ReadRaw(is, count))
        return false;

    v.resize((size_t)count);
    for (auto& id : v)
        if (!ReadAssetId(is, id))
            return false;

    return true;
}

template <typename T> bool WritePodVector(std::ostream& os, const std::vector<T>& v)
{
    std::uint64_t count = v.size();
    if (!WriteRaw(os, count))
        return false;

    if (!v.empty())
    {
        os.write(reinterpret_cast<const char*>(v.data()), sizeof(T) * v.size());
        if (!os)
            return false;
    }
    return true;
}

template <typename T> bool ReadPodVector(std::istream& is, std::vector<T>& v)
{
    std::uint64_t count = 0;
    if (!ReadRaw(is, count))
        return false;

    v.resize((size_t)count);
    if (!v.empty())
    {
        is.read(reinterpret_cast<char*>(v.data()), sizeof(T) * v.size());
        if (!is)
            return false;
    }
    return true;
}

bool WriteImportedImage(std::ostream& os, const ImportedImage& v)
{
    return WriteAssetId(os, v.id) && WriteString(os, v.debugName) && WriteRaw(os, v.width) && WriteRaw(os, v.height) &&
           WritePodVector(os, v.rgba8) && WriteRaw(os, v.channels) && WriteRaw(os, v.srgb);
}

bool ReadImportedImage(std::istream& is, ImportedImage& v)
{
    return ReadAssetId(is, v.id) && ReadString(is, v.debugName) && ReadRaw(is, v.width) && ReadRaw(is, v.height) &&
           ReadPodVector(is, v.rgba8) && ReadRaw(is, v.channels) && ReadRaw(is, v.srgb);
}

bool WriteImportedTexture(std::ostream& os, const ImportedTexture& v)
{
    return WriteAssetId(os, v.id) && WriteString(os, v.debugName) && WriteAssetId(os, v.imageId) &&
           WriteRaw(os, v.samplerStateWrapU) && WriteRaw(os, v.samplerStateWrapV) &&
           WriteRaw(os, v.samplerStateMinFilter) && WriteRaw(os, v.samplerStateMagFilter);
}

bool ReadImportedTexture(std::istream& is, ImportedTexture& v)
{
    return ReadAssetId(is, v.id) && ReadString(is, v.debugName) && ReadAssetId(is, v.imageId) &&
           ReadRaw(is, v.samplerStateWrapU) && ReadRaw(is, v.samplerStateWrapV) &&
           ReadRaw(is, v.samplerStateMinFilter) && ReadRaw(is, v.samplerStateMagFilter);
}

bool WriteImportedMaterial(std::ostream& os, const ImportedMaterial& v)
{
    return WriteAssetId(os, v.id) && WriteString(os, v.debugName) && WriteRaw(os, v.baseColorFactor) &&
           WriteRaw(os, v.metallic) && WriteRaw(os, v.roughness) && WriteRaw(os, v.precompF0) &&
           WriteRaw(os, v.normalScalar) && WriteRaw(os, v.emissiveFactor) && WriteRaw(os, v.doubleSided) &&
           WriteRaw(os, v.flipNormals) && WriteAssetId(os, v.baseColorTex) && WriteAssetId(os, v.normalTex) &&
           WriteAssetId(os, v.mrTex) && WriteAssetId(os, v.emissiveTex) && WriteAssetId(os, v.specularTex) &&
           WriteAssetId(os, v.specularColorTex) && WriteRaw(os, v.flags) && WriteRaw(os, v.blendingMode) &&
           WriteRaw(os, v.renderingMode);
}

bool ReadImportedMaterial(std::istream& is, ImportedMaterial& v)
{
    return ReadAssetId(is, v.id) && ReadString(is, v.debugName) && ReadRaw(is, v.baseColorFactor) &&
           ReadRaw(is, v.metallic) && ReadRaw(is, v.roughness) && ReadRaw(is, v.precompF0) &&
           ReadRaw(is, v.normalScalar) && ReadRaw(is, v.emissiveFactor) && ReadRaw(is, v.doubleSided) &&
           ReadRaw(is, v.flipNormals) && ReadAssetId(is, v.baseColorTex) && ReadAssetId(is, v.normalTex) &&
           ReadAssetId(is, v.mrTex) && ReadAssetId(is, v.emissiveTex) && ReadAssetId(is, v.specularTex) &&
           ReadAssetId(is, v.specularColorTex) && ReadRaw(is, v.flags) && ReadRaw(is, v.blendingMode) &&
           ReadRaw(is, v.renderingMode);
}

bool WriteSubMesh(std::ostream& os, const ImportedMesh::SubMesh& v)
{
    return WriteRaw(os, v.indexOffset) && WriteRaw(os, v.indexCount) && WriteRaw(os, v.materialId);
}

bool ReadSubMesh(std::istream& is, ImportedMesh::SubMesh& v)
{
    return ReadRaw(is, v.indexOffset) && ReadRaw(is, v.indexCount) && ReadRaw(is, v.materialId);
}

bool WriteImportedMesh(std::ostream& os, const ImportedMesh& v)
{
    if (!WriteAssetId(os, v.id) || !WriteString(os, v.debugName) || !WritePodVector(os, v.positions) ||
        !WritePodVector(os, v.normals) || !WritePodVector(os, v.tangents) || !WritePodVector(os, v.uv0) ||
        !WritePodVector(os, v.colors) || !WritePodVector(os, v.indices) || !WritePodVector(os, v.joints_0) ||
        !WritePodVector(os, v.joints_1) || !WritePodVector(os, v.weights_0) || !WritePodVector(os, v.weights_1))
        return false;

    std::uint64_t count = v.subMeshes.size();
    if (!WriteRaw(os, count))
        return false;

    for (const auto& sm : v.subMeshes)
        if (!WriteSubMesh(os, sm))
            return false;

    return true;
}

bool ReadImportedMesh(std::istream& is, ImportedMesh& v)
{
    if (!ReadAssetId(is, v.id) || !ReadString(is, v.debugName) || !ReadPodVector(is, v.positions) ||
        !ReadPodVector(is, v.normals) || !ReadPodVector(is, v.tangents) || !ReadPodVector(is, v.uv0) ||
        !ReadPodVector(is, v.colors) || !ReadPodVector(is, v.indices) || !ReadPodVector(is, v.joints_0) ||
        !ReadPodVector(is, v.joints_1) || !ReadPodVector(is, v.weights_0) || !ReadPodVector(is, v.weights_1))
        return false;

    std::uint64_t count = 0;
    if (!ReadRaw(is, count))
        return false;

    v.subMeshes.resize((size_t)count);
    for (auto& sm : v.subMeshes)
        if (!ReadSubMesh(is, sm))
            return false;

    return true;
}

bool WriteImportedNode(std::ostream& os, const ImportedNode& v)
{
    return WriteAssetId(os, v.NodeId) && WriteRaw(os, v.parent) && WritePodVector(os, v.children) &&
           WriteString(os, v.debugName) && WriteRaw(os, v.t) && WriteRaw(os, v.r) && WriteRaw(os, v.s) &&
           WriteAssetId(os, v.meshId) && WriteRaw(os, v.skinIndex) && WriteAssetIdVector(os, v.materialId);
}

bool ReadImportedNode(std::istream& is, ImportedNode& v)
{
    return ReadAssetId(is, v.NodeId) && ReadRaw(is, v.parent) && ReadPodVector(is, v.children) &&
           ReadString(is, v.debugName) && ReadRaw(is, v.t) && ReadRaw(is, v.r) && ReadRaw(is, v.s) &&
           ReadAssetId(is, v.meshId) && ReadRaw(is, v.skinIndex) && ReadAssetIdVector(is, v.materialId);
}

} // namespace

bool ImportedSourceStore::SaveModel(const AssetId& sourceId, const ImportedModel& model)
{
    fs::path path = ProjectManager::GetInstance().GetCurrentProjectPath() + "/EngineCache/Imported/" + sourceId.to_string() + ".bin";

    fs::create_directories(path.parent_path());

    std::ofstream os(path, std::ios::binary | std::ios::trunc);
    if (!os)
        return false;

    if (!WriteRaw(os, kMagic) || !WriteRaw(os, kVersion) || !WriteAssetId(os, model.modelId) ||
        !WriteString(os, model.sourcePath) || !WriteString(os, model.debugName))
        return false;

    auto writeArray = [&](auto& vec, auto fn)
    {
        std::uint64_t count = vec.size();
        if (!WriteRaw(os, count))
            return false;
        for (auto& v : vec)
            if (!fn(os, v))
                return false;
        return true;
    };

    if (!writeArray(model.images, WriteImportedImage))
        return false;
    if (!writeArray(model.textures, WriteImportedTexture))
        return false;
    if (!writeArray(model.materials, WriteImportedMaterial))
        return false;
    if (!writeArray(model.meshes, WriteImportedMesh))
        return false;
    if (!writeArray(model.nodes, WriteImportedNode))
        return false;

    bool hasRig = model.rig.has_value();
    if (!WriteRaw(os, hasRig))
        return false;

    if (hasRig)
    {
        if (!WriteAssetId(os, model.rig->rigId) || !WriteAssetIdVector(os, model.rig->joints) ||
            !WritePodVector(os, model.rig->skinIndices))
            return false;
    }

    if (!writeArray(model.skins,
                    [](auto& os, const ImportedSkin& s)
                    {
                        return WritePodVector(os, s.jointIndices) && WritePodVector(os, s.inverseBindMatrices) &&
                               WriteRaw(os, s.skeleton);
                    }))
        return false;

    if (!WritePodVector(os, model.rootNodes))
        return false;

    return true;
}

std::optional<ImportedModel> ImportedSourceStore::LoadModel(const AssetId& sourceId)
{
    fs::path path = ProjectManager::GetInstance().GetCurrentProjectPath() + "/EngineCache/Imported/" +
                    sourceId.to_string() + ".bin";

    std::ifstream is(path, std::ios::binary);
    if (!is)
        return std::nullopt;

    std::uint32_t magic = 0, version = 0;
    if (!ReadRaw(is, magic) || !ReadRaw(is, version))
        return std::nullopt;
    if (magic != kMagic || version != kVersion)
        return std::nullopt;

    ImportedModel model{};

    if (!ReadAssetId(is, model.modelId) || !ReadString(is, model.sourcePath) || !ReadString(is, model.debugName))
        return std::nullopt;

    auto readArray = [&](auto& vec, auto fn)
    {
        std::uint64_t count = 0;
        if (!ReadRaw(is, count))
            return false;
        vec.resize((size_t)count);
        for (auto& v : vec)
            if (!fn(is, v))
                return false;
        return true;
    };

    if (!readArray(model.images, ReadImportedImage))
        return std::nullopt;
    if (!readArray(model.textures, ReadImportedTexture))
        return std::nullopt;
    if (!readArray(model.materials, ReadImportedMaterial))
        return std::nullopt;
    if (!readArray(model.meshes, ReadImportedMesh))
        return std::nullopt;
    if (!readArray(model.nodes, ReadImportedNode))
        return std::nullopt;

    bool hasRig = false;
    if (!ReadRaw(is, hasRig))
        return std::nullopt;

    if (hasRig)
    {
        model.rig.emplace();
        if (!ReadAssetId(is, model.rig->rigId) || !ReadAssetIdVector(is, model.rig->joints) ||
            !ReadPodVector(is, model.rig->skinIndices))
            return std::nullopt;
    }

    if (!readArray(model.skins,
                   [](auto& is, ImportedSkin& s)
                   {
                       return ReadPodVector(is, s.jointIndices) && ReadPodVector(is, s.inverseBindMatrices) &&
                              ReadRaw(is, s.skeleton);
                   }))
        return std::nullopt;

    if (!ReadPodVector(is, model.rootNodes))
        return std::nullopt;

    return model;
}
} // namespace REON::EDITOR
