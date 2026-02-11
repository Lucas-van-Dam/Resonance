#include "reonpch.h"
#define TINYGLTF_IMPLEMENTATION
#include "GLTFProcessor.h"
#include "mikktspace.h"

#include <ProjectManagement/ProjectManager.h>
#include <REON/ResourceManagement/ResourceInfo.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

using json = nlohmann::json;

namespace REON::EDITOR
{

void GLTFProcessor::Process(AssetInfo& assetInfo)
{
    tg::Model model;
    tg::TinyGLTF loader;
    std::string err;
    std::string warn;

    auto extension = assetInfo.path.extension().string();
    fullPath = ProjectManager::GetInstance().GetCurrentProjectPath() + "\\" + assetInfo.path.string();

    if (extension == ".gltf")
    {
        if (!loader.LoadASCIIFromFile(&model, &err, &warn, fullPath.string()))
        {
            REON_ERROR("Failed to parse GLTF: {}", err);
        }
    }
    else if (extension == ".glb")
    {
        if (!loader.LoadBinaryFromFile(&model, &err, &warn, fullPath.string()))
        {
            REON_ERROR("Failed to parse GLTF: {}", err);
        }
    }
    else
        REON_ERROR("File was not recognised as a gltf file.");

    if (!err.empty())
    {
        REON_ERROR("Error while opening gltf file: {}", err);
    }

    if (!warn.empty())
    {
        REON_WARN("Warning while opening gltf file: {}", warn);
    }

    uid = assetInfo.id;
    basePath = assetInfo.path;

    MetaFileData metaData;
    metaData.modelUID = assetInfo.id;

    metaData.originPath = basePath.string();

    // std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>();

    // ResourceManager::GetInstance().AddResource(mesh);

    materialIDs.clear();

    if (model.extensionsUsed.size() > 0)
        REON_INFO("Model: {} uses extensions:", basePath.string());
    for (auto khrExtension : model.extensionsUsed)
    {
        REON_INFO("\t{}", khrExtension);
    }

    for (auto& srcMat : model.materials)
    {
        auto litShader = ResourceManager::GetInstance().LoadResource<Shader>(
            "DefaultLit", std::make_tuple("PBR.vert", "PBR.frag", std::optional<std::string>{}));

        std::shared_ptr<Material> mat = std::make_shared<Material>();
        mat->shader = litShader;

        mat->SetName(srcMat.name.empty() ? "Material_" + mat->GetID() : srcMat.name);

        materialIDs.push_back(mat->GetID());

        uint32_t flags = 0;

        if (srcMat.alphaMode == "OPAQUE")
        {
            mat->renderingMode = Opaque;
        }
        else if (srcMat.alphaMode == "MASK")
        {
            mat->renderingMode = Transparent;
            mat->blendingMode = Mask;
            flags |= AlphaCutoff;
        }
        else if (srcMat.alphaMode == "BLEND")
        {
            mat->renderingMode = Transparent;
            mat->blendingMode = Blend;
        }
        else
        {
            REON_WARN("Unknown alpha mode, material: {}", srcMat.name);
        }

        mat->flatData.emissiveFactor.w = srcMat.alphaCutoff;

        mat->setDoubleSided(srcMat.doubleSided);

        auto pbrData = srcMat.pbrMetallicRoughness;

        mat->flatData.albedo =
            glm::vec4(static_cast<float>(pbrData.baseColorFactor[0]), static_cast<float>(pbrData.baseColorFactor[1]),
                      static_cast<float>(pbrData.baseColorFactor[2]), static_cast<float>(pbrData.baseColorFactor[3]));

        if (pbrData.baseColorTexture.index >= 0)
        {

            mat->albedoTexture = HandleGLTFTexture(model, model.textures[pbrData.baseColorTexture.index],
                                                   VK_FORMAT_R8G8B8A8_SRGB, pbrData.baseColorTexture.index);
            flags |= AlbedoTexture;
            if (pbrData.baseColorTexture.texCoord != 0)
                REON_WARN("Albedo texture has non 0 texcoord and is not used");
        }

        mat->flatData.roughness = pbrData.roughnessFactor;
        if (mat->flatData.roughness < 0 || mat->flatData.roughness > 1)
        {
            mat->flatData.roughness = 1.0;
        }
        mat->flatData.metallic = pbrData.metallicFactor;
        if (mat->flatData.metallic < 0 || mat->flatData.metallic > 1)
        {
            mat->flatData.metallic = 0.0;
        }

        auto emissiveFactor = srcMat.emissiveFactor;
        mat->flatData.emissiveFactor.r = emissiveFactor[0];
        mat->flatData.emissiveFactor.g = emissiveFactor[1];
        mat->flatData.emissiveFactor.b = emissiveFactor[2];

        for (auto khrExtension : srcMat.extensions)
        {
            if (khrExtension.first == "KHR_materials_emissive_strength")
            {
                float emissiveStrength = khrExtension.second.Get("emissiveStrength").GetNumberAsDouble();
                mat->flatData.emissiveFactor.r *= emissiveStrength;
                mat->flatData.emissiveFactor.g *= emissiveStrength;
                mat->flatData.emissiveFactor.b *= emissiveStrength;
            }
            else if (khrExtension.first == "KHR_materials_ior")
            {
                float ior = khrExtension.second.Get("ior").GetNumberAsDouble();
                mat->flatData.preCompF0 = pow(((ior - 1.0f) / (ior + 1.0f)), 2.0f);
            }
            // else if (khrExtension.first == "KHR_materials_specular") {
            //	if (khrExtension.second.Has("specularFactor")) {
            //		mat->flatData.specularFactor.a = khrExtension.second.Get("specularFactor").GetNumberAsDouble();
            //	}
            //	else {
            //		continue;
            //	}
            //	flags |= Specular;
            // }
            else
            {
                REON_WARN("Extension: {} is not supported yet", khrExtension.first);
            }
        }

        if (srcMat.emissiveTexture.index >= 0)
        {
            mat->emissiveTexture = HandleGLTFTexture(model, model.textures[srcMat.emissiveTexture.index],
                                                     VK_FORMAT_R8G8B8A8_SRGB, srcMat.emissiveTexture.index);
            flags |= EmissiveTexture;
            if (srcMat.emissiveTexture.texCoord != 0)
                REON_WARN("Emission texture has non 0 texcoord and is not used");
        }

        if (pbrData.metallicRoughnessTexture.index >= 0)
        {
            mat->metallicRoughnessTexture =
                HandleGLTFTexture(model, model.textures[pbrData.metallicRoughnessTexture.index],
                                  VK_FORMAT_R8G8B8A8_SRGB, pbrData.metallicRoughnessTexture.index);
            flags |= MetallicRoughnessTexture;
            if (pbrData.metallicRoughnessTexture.texCoord != 0)
                REON_WARN("MetallicRoughness texture has non 0 texcoord and is not used");
        }

        if (srcMat.normalTexture.index >= 0)
        {
            mat->normalTexture = HandleGLTFTexture(model, model.textures[srcMat.normalTexture.index],
                                                   VK_FORMAT_R8G8B8A8_UNORM, srcMat.normalTexture.index);
            flags |= NormalTexture;
            mat->flatData.normalScalar = srcMat.normalTexture.scale;
            if (srcMat.normalTexture.texCoord != 0)
                REON_WARN("Normal texture has non 0 texcoord and is not used");
        }

        mat->materialFlags = flags;

        AssetRegistry::Instance().RegisterAsset({mat->GetID(), "Material",
                                                 mat->Serialize(ProjectManager::GetInstance().GetCurrentProjectPath() +
                                                                "\\" + (basePath.parent_path().string()))
                                                     .string()});

        ResourceManager::GetInstance().AddResource(mat);
    }

    const int sceneToLoad = model.defaultScene < 0 ? 0 : model.defaultScene;

    metaData.sceneName = model.scenes[sceneToLoad].name;

    std::unordered_map<int, std::string> nodeIdtoUuid;

    for (auto& nodeId : model.scenes[sceneToLoad].nodes)
    {
        if (nodeId < 0)
            return;

        metaData.rootNodes.push_back(
            HandleGLTFNode(model, nodeId, glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1, 0, 0)),
                           metaData, nodeIdtoUuid)); // pre-rotation for correction of -90 degree rotation
    }

    for (auto& anim : model.animations)
    {
    }

    assetInfo.extraInformation["Importer"] = "GLTFImporter";

    std::string metaDataPath = ProjectManager::GetInstance().GetCurrentProjectPath() + "\\" +
                               assetInfo.path.parent_path().string() + "\\" + assetInfo.path.stem().string() + ".model";

    SerializeCompanionFile(metaData, metaDataPath);
}

SceneNodeData GLTFProcessor::HandleGLTFNode(const tg::Model& model, int nodeId, const glm::mat4& parentTransform,
                                            MetaFileData& modelFileData,
                                            std::unordered_map<int, std::string>& nodeIdToUuId)
{
    auto node = model.nodes[nodeId];
    SceneNodeData data;
    data.name = node.name;
    // TODO: use an engine-level function, this is just ugly and bad practice
    data.nodeUUID = Object::GenerateUUID();

    nodeIdToUuId[nodeId] = data.nodeUUID;

    glm::mat4 transform = GetTransformFromGLTFNode(node);

    auto trs = GetTRSFromGLTFNode(node);
    data.translation = std::get<0>(trs);
    data.rotation = std::get<1>(trs);
    data.scale = std::get<2>(trs);

    data.transform = transform;

    int meshId = node.mesh;

    if (meshId >= 0 && meshId < model.meshes.size())
    {
        auto meshID = HandleGLTFMesh(model, model.meshes[meshId], transform, data);
        data.meshIDs.push_back(meshID);
        modelFileData.meshes.push_back(ResourceManager::GetInstance().GetResource<Mesh>(meshID));
    }

    for (auto& childId : node.children)
    {
        if (childId < 0 || childId > model.nodes.size())
            continue;

        data.children.push_back(HandleGLTFNode(model, childId, transform, modelFileData, nodeIdToUuId));
    }

    return data;
}

std::string GLTFProcessor::HandleGLTFMesh(const tg::Model& model, const tg::Mesh& mesh, const glm::mat4& transform,
                                          SceneNodeData& sceneNode)
{
    std::shared_ptr<Mesh> meshData = std::make_shared<Mesh>();
    meshData->SetName(mesh.name);

    std::vector<std::string> matIDsPerMesh;

    int primitiveIndex = 0;
    for (auto primitive : mesh.primitives)
    {
        SubMesh subMesh;
        subMesh.indexOffset = meshData->indices.size();

        if (primitive.material >= 0)
        {
            auto matID = materialIDs[primitive.material];
            auto it = std::find(matIDsPerMesh.begin(), matIDsPerMesh.end(), matID);
            if (it != matIDsPerMesh.end())
            {
                subMesh.materialIndex = std::distance(matIDsPerMesh.begin(), it);
            }
            else
            {
                subMesh.materialIndex = matIDsPerMesh.size();
                matIDsPerMesh.push_back(matID);
            }
            sceneNode.materials.push_back(matID);
        }
        else
        {
            subMesh.materialIndex = -1;
        }

        const int vertexOffset = meshData->positions.size();

        for (auto& attribute : primitive.attributes)
        {
            const tinygltf::Accessor& accessor = model.accessors.at(attribute.second);
            if (attribute.first.compare("POSITION") == 0)
            {
                HandleGLTFBuffer(model, accessor, meshData->positions, transform);
            }
            else if (attribute.first.compare("NORMAL") == 0)
            {
                HandleGLTFBuffer(model, accessor, meshData->normals, transform, true);
            }
            else if (attribute.first.compare("TEXCOORD_0") == 0)
            {
                HandleGLTFBuffer(model, accessor, meshData->texCoords);
            }
            else if (attribute.first.compare("TANGENT") == 0)
            {
                HandleGLTFBuffer(model, accessor, meshData->tangents);
            }
            else if (attribute.first.compare("COLOR_0") == 0)
            {
                REON_WARN("Color found, but not implemented yet");
                // auto result = detail::handleGltfVertexColor(model, accessor, accessor.type, accessor.componentType,
                // meshData.colors);
            }
            else
            {
                REON_WARN("More data in GLTF file, accessor name: {}", attribute.first);
            }
        }

        const int smallestBufferSize =
            std::min(meshData->normals.size(), std::min(meshData->texCoords.size(), meshData->colors.size()));
        const int vertexCount = meshData->positions.size();
        meshData->normals.reserve(vertexCount);
        meshData->texCoords.reserve(vertexCount);
        meshData->colors.reserve(vertexCount);

        for (int i = smallestBufferSize; i < vertexCount; ++i)
        {
            if (meshData->normals.size() == i)
                meshData->normals.push_back(glm::vec3(0, 1, 0));

            if (meshData->texCoords.size() == i)
                meshData->texCoords.push_back(glm::vec2(0, 0));

            if (meshData->colors.size() == i)
                meshData->colors.push_back(glm::vec4(1, 1, 1, 1));
        }

        const int index = primitive.indices;

        if (index > model.accessors.size())
        {
            REON_WARN("Invalid index accessor index in primitive: {}", std::to_string(primitiveIndex));
            return "";
        }

        if (primitive.indices >= 0)
        {
            const tg::Accessor& indexAccessor = model.accessors.at(index);
            HandleGLTFIndices(model, indexAccessor, vertexOffset, meshData->indices);
        }
        else
        {
            for (uint i = static_cast<uint>(vertexOffset); i < meshData->positions.size(); i++)
                meshData->indices.push_back(i);
        }
        subMesh.indexCount = meshData->indices.size() - subMesh.indexOffset;

        primitiveIndex++;

        meshData->subMeshes.push_back(subMesh);
    }

    if (meshData->tangents.empty())
    {
        Mesh::calculateTangents(meshData);
    }

    ResourceManager::GetInstance().AddResource(meshData);
    AssetRegistry::Instance().RegisterAsset({meshData->GetID(), "Mesh", basePath.string() + ".meta"});

    return meshData->GetID();
}

glm::mat4 GLTFProcessor::GetTransformFromGLTFNode(const tg::Node& node)
{
    if (node.matrix.size() == 16)
    {
        return glm::mat4(static_cast<float>(node.matrix[0]), static_cast<float>(node.matrix[1]),
                         static_cast<float>(node.matrix[2]), static_cast<float>(node.matrix[3]),
                         static_cast<float>(node.matrix[4]), static_cast<float>(node.matrix[5]),
                         static_cast<float>(node.matrix[6]), static_cast<float>(node.matrix[7]),
                         static_cast<float>(node.matrix[8]), static_cast<float>(node.matrix[9]),
                         static_cast<float>(node.matrix[10]), static_cast<float>(node.matrix[11]),
                         static_cast<float>(node.matrix[12]), static_cast<float>(node.matrix[13]),
                         static_cast<float>(node.matrix[14]), static_cast<float>(node.matrix[15]));
    }
    else
    {
        glm::vec3 trans{0, 0, 0};
        Quaternion rot{1, 0, 0, 0};
        glm::vec3 scale{1, 1, 1};

        if (node.scale.size() == 3)
            scale = glm::vec3(static_cast<float>(node.scale[0]), static_cast<float>(node.scale[1]),
                              static_cast<float>(node.scale[2]));

        if (node.rotation.size() == 4)
            rot = Quaternion(static_cast<float>(node.rotation[3]), static_cast<float>(node.rotation[0]),
                             static_cast<float>(node.rotation[1]), static_cast<float>(node.rotation[2]));

        if (node.translation.size() == 3)
            trans = glm::vec3(static_cast<float>(node.translation[0]), static_cast<float>(node.translation[1]),
                              static_cast<float>(node.translation[2]));

        glm::mat4 translation = glm::translate(glm::mat4(1.0f), trans);
        glm::mat4 rotation = glm::toMat4(rot);
        glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), scale);

        return (translation * rotation * scaleMat);
    }
}

std::tuple<glm::vec3, Quaternion, glm::vec3> GLTFProcessor::GetTRSFromGLTFNode(const tg::Node& node)
{
    glm::vec3 trans{0, 0, 0};
    Quaternion rot{1, 0, 0, 0};
    glm::vec3 scale{1, 1, 1};
    if (node.matrix.size() == 16)
    {
        glm::mat4 mat = GetTransformFromGLTFNode(node);
        REON_WARN("Node has a matrix, but decomposition to TRS is not implemented yet, returning default values");
        // TODO: decompose matrix to get TRS, currently just return default values
        return {trans, rot, scale};
    }
    else
    {
        glm::vec3 trans{0, 0, 0};
        Quaternion rot{1, 0, 0, 0};
        glm::vec3 scale{1, 1, 1};

        if (node.scale.size() == 3)
            scale = glm::vec3(static_cast<float>(node.scale[0]), static_cast<float>(node.scale[1]),
                              static_cast<float>(node.scale[2]));

        if (node.rotation.size() == 4)
            rot = Quaternion(static_cast<float>(node.rotation[3]), static_cast<float>(node.rotation[0]),
                             static_cast<float>(node.rotation[1]), static_cast<float>(node.rotation[2]));

        if (node.translation.size() == 3)
            trans = glm::vec3(static_cast<float>(node.translation[0]), static_cast<float>(node.translation[1]),
                              static_cast<float>(node.translation[2]));

        return {trans, rot, scale};
    }
}

void GLTFProcessor::HandleGLTFBuffer(const tg::Model& model, const tg::Accessor& accessor, std::vector<glm::vec3>& data,
                                     const glm::mat4& transform, bool normal)
{
    const tg::BufferView& bufferView = model.bufferViews.at(accessor.bufferView);
    const tg::Buffer& buffer = model.buffers.at(bufferView.buffer);
    const int bufferStart = bufferView.byteOffset + accessor.byteOffset;
    const int stride = accessor.ByteStride(bufferView);
    const int bufferEnd = bufferStart + accessor.count * stride;

    const int dataStartAmount = data.size();
    data.reserve(dataStartAmount + accessor.count);

    if (normal)
    {
        for (size_t i = bufferStart; i < bufferEnd; i += stride)
        {
            const float* x = reinterpret_cast<const float*>(&buffer.data[i]);
            const float* y = reinterpret_cast<const float*>(&buffer.data[i + sizeof(float)]);
            const float* z = reinterpret_cast<const float*>(&buffer.data[i + 2 * sizeof(float)]);
            data.push_back(glm::vec3((/*transform **/ glm::vec4(*x, *y, *z, 0.f))));
        }
    }
    else
    {
        for (size_t i = bufferStart; i < bufferEnd; i += stride)
        {
            const float* x = reinterpret_cast<const float*>(&buffer.data[i]);
            const float* y = reinterpret_cast<const float*>(&buffer.data[i + sizeof(float)]);
            const float* z = reinterpret_cast<const float*>(&buffer.data[i + 2 * sizeof(float)]);
            data.push_back(glm::vec3((/*transform **/ glm::vec4(*x, *y, *z, 1.f))));
        }
    }

    if (accessor.sparse.isSparse)
        REON_WARN("Sparse accessors are not supported yet (while processing buffer)");
}

template <typename T>
void GLTFProcessor::HandleGLTFBuffer(const tinygltf::Model& model, const tinygltf::Accessor& accessor,
                                     std::vector<T>& data)
{
    const tg::BufferView& bufferView = model.bufferViews.at(accessor.bufferView);
    const tg::Buffer& buffer = model.buffers.at(bufferView.buffer);
    const int bufferStart = bufferView.byteOffset + accessor.byteOffset;
    const int stride = accessor.ByteStride(bufferView);
    const int bufferEnd = bufferStart + accessor.count * stride;

    const int dataStart = data.size();
    data.reserve(dataStart + accessor.count);

    for (size_t i = bufferStart; i < bufferEnd; i += stride)
        data.push_back(*reinterpret_cast<const T*>(&buffer.data[i]));

    if (accessor.sparse.isSparse)
        REON_WARN("Sparse accessors are not supported yet (while processing template buffer)");
}

void GLTFProcessor::HandleGLTFIndices(const tg::Model& model, const tg::Accessor& accessor, int offset,
                                      std::vector<uint>& data)
{
    const tinygltf::BufferView& bufferView = model.bufferViews.at(accessor.bufferView);
    const tinygltf::Buffer& buffer = model.buffers.at(bufferView.buffer);
    const int bufferStart = bufferView.byteOffset + accessor.byteOffset;
    const int stride = accessor.ByteStride(bufferView);
    const int bufferEnd = bufferStart + accessor.count * stride;

    const int dataStart = data.size();
    data.reserve(dataStart + accessor.count);

    switch (accessor.componentType)
    {
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
        for (int i = bufferStart; i < bufferEnd; i += stride)
            data.push_back(static_cast<uint>(buffer.data[i] + offset));
        break;

    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
        for (int i = bufferStart; i < bufferEnd; i += stride)
            data.push_back(static_cast<uint>(*reinterpret_cast<const UINT16*>(&buffer.data[i]) + offset));
        break;

    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
        for (int i = bufferStart; i < bufferEnd; i += stride)
            data.push_back(*reinterpret_cast<const uint*>(&buffer.data[i]) + static_cast<uint>(offset));
        break;
    default:
        REON_WARN("Index component type not supported: {}", accessor.componentType);
    }

    if (accessor.sparse.isSparse)
        REON_WARN("Sparse accessors are not supported yet (while processing indices)");
}

nlohmann::ordered_json GLTFProcessor::SerializeSceneNode(const SceneNodeData& node)
{
    nlohmann::ordered_json j;
    j["name"] = node.name;
    // Serialize the transform matrix as a flat array (16 floats)
    std::vector<float> transformData(16);
    const float* ptr = glm::value_ptr(node.transform);
    std::copy(ptr, ptr + 16, transformData.begin());
    j["transform"] = transformData;
    nlohmann::ordered_json trs;
    trs["translation"] = {node.translation.x, node.translation.y, node.translation.z};
    trs["rotation"] = {node.rotation.x, node.rotation.y, node.rotation.z, node.rotation.w};
    trs["scale"] = {node.scale.x, node.scale.y, node.scale.z};
    j["localTRS"] = trs;
    j["meshIDs"] = node.meshIDs;
    j["materials"] = node.materials;

    // Process children recursively.
    nlohmann::ordered_json children = nlohmann::ordered_json::array();
    for (const auto& child : node.children)
    {
        children.push_back(SerializeSceneNode(child));
    }
    j["children"] = children;
    return j;
}

void GLTFProcessor::SerializeCompanionFile(const MetaFileData& data, const std::string& outPath)
{
    nlohmann::ordered_json j;
    j["GUID"] = data.modelUID;
    j["Origin"] = data.originPath;
    j["sceneName"] = data.sceneName;

    nlohmann::ordered_json nodeArray = nlohmann::ordered_json::array();
    for (auto node : data.rootNodes)
    {
        nodeArray.push_back(SerializeSceneNode(node));
    }
    j["rootNodes"] = nodeArray;

    // Convert mesh data to JSON
    nlohmann::ordered_json meshArray = nlohmann::ordered_json::array();
    for (auto mesh : data.meshes) // Assuming `data.meshes` is a list of Mesh objects
    {
        meshArray.push_back(mesh->Serialize());
    }

    j["meshes"] = meshArray;

    std::ofstream file(outPath);
    if (file.is_open())
    {
        file << j.dump(4); // Pretty print with an indent of 4 spaces.
        file.close();
    }
    else
    {
        REON_ERROR("Failed to open companion file for writing: {}", outPath);
    }
}

std::shared_ptr<Texture> GLTFProcessor::HandleGLTFTexture(const tg::Model& model, const tg::Texture& tgTexture,
                                                          VkFormat format, int textureIndex)
{
    const auto& image = model.images[tgTexture.source];

    auto path = image.uri.empty() ? basePath.stem().string() + "_texture_" + std::to_string(textureIndex) : image.uri;

    auto outPath = basePath.parent_path().string() + "\\" + path + ".img";

    TextureHeader header{};
    header.width = image.width;
    header.height = image.height;
    header.channels = image.component;
    header.format = format;
    // I know its ugly, but whatever, ill change it later
    if (tgTexture.sampler >= 0)
    {
        const auto& sampler = model.samplers[tgTexture.sampler];

        switch (sampler.magFilter)
        {
        case TINYGLTF_TEXTURE_FILTER_NEAREST:
            header.samplerData.magFilter = VK_FILTER_NEAREST;
            break;
        case TINYGLTF_TEXTURE_FILTER_LINEAR:
            header.samplerData.magFilter = VK_FILTER_LINEAR;
            break;
        default:
            REON_WARN("Unknown magfilter on texture {}", sampler.magFilter);
            break;
        }
        switch (sampler.minFilter)
        {
        case TINYGLTF_TEXTURE_FILTER_NEAREST:
            header.samplerData.minFilter = VK_FILTER_NEAREST;
            break;
        case TINYGLTF_TEXTURE_FILTER_LINEAR:
            header.samplerData.minFilter = VK_FILTER_LINEAR;
            break;
        case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
            header.samplerData.minFilter = VK_FILTER_NEAREST;
            break;
        case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
            header.samplerData.minFilter = VK_FILTER_NEAREST;
            break;
        case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
            header.samplerData.minFilter = VK_FILTER_LINEAR;
            break;
        case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
            header.samplerData.minFilter = VK_FILTER_LINEAR;
            break;
        default:
            REON_WARN("Unknown minFilter on texture {}", sampler.minFilter);
            break;
        }
        switch (sampler.wrapS)
        {
        case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
            header.samplerData.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            break;
        case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
            header.samplerData.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
            break;
        case TINYGLTF_TEXTURE_WRAP_REPEAT:
            header.samplerData.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            break;
        default:
            REON_WARN("Unknown wrap U on texture {}", sampler.wrapS);
            break;
        }
        switch (sampler.wrapT)
        {
        case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
            header.samplerData.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            break;
        case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
            header.samplerData.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
            break;
        case TINYGLTF_TEXTURE_WRAP_REPEAT:
            header.samplerData.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            break;
        default:
            REON_WARN("Unknown wrap V on texture {}", sampler.wrapT);
            break;
        }
    }
    std::ofstream out(fullPath.parent_path().string() + "\\" + path + ".img", std::ios::binary);
    if (out.is_open())
    {
        out.write(reinterpret_cast<const char*>(&header), sizeof(TextureHeader));
        out.write(reinterpret_cast<const char*>(image.image.data()), image.image.size() * sizeof(image.image[0]));
        out.close();
    }

    auto texture = ResourceManager::GetInstance().LoadResource<Texture>(outPath, std::make_tuple(header, image.image));

    AssetInfo textureInfo{};
    textureInfo.id = texture->GetID();
    textureInfo.type = "Texture";
    textureInfo.path = outPath;

    nlohmann::ordered_json json;

    json["Id"] = textureInfo.id;
    json["Type"] = textureInfo.type;
    json["Path"] = textureInfo.path.string();
    json["Extra Data"] = textureInfo.extraInformation;

    std::ofstream metaOut(fullPath.parent_path().string() + "\\" + path + ".img.meta");
    if (metaOut.is_open())
    {
        metaOut << json.dump(4);
        metaOut.close();
    }

    AssetRegistry::Instance().RegisterAsset(textureInfo);
    return texture;
}

// void GLTFProcessor::ProcessNode(aiNode* node, const aiScene* scene, std::string path) {
//	if (node->mNumMeshes > 0) {
//		for (int i = 0; i < node->mNumMeshes; i++) {
//			ProcessMesh(scene->mMeshes[node->mMeshes[i]], scene, path);
//		}
//	}

//	for (int i = 0; i < node->mNumChildren; i++) {
//		ProcessNode(node->mChildren[i], scene, path);
//	}
//}

// void GLTFProcessor::ProcessMesh(aiMesh* mesh, const aiScene* scene, std::string path) {
//	std::vector<Vertex> vertices;
//	std::vector<unsigned int> indices;
//	std::string meshIdentifier = path.substr(0, path.find_last_of('/')) + "/" + mesh->mName.C_Str();

//	std::shared_ptr<Mesh> meshObj = ResourceManager::GetInstance().LoadResource<Mesh>(meshIdentifier,
//std::make_tuple(vertices, indices));

//	meshObj->Serialize(ProjectManager::GetInstance().GetCurrentProjectPath() + "\\EngineCache\\Meshes");

//	ResourceInfo info;
//	info.path = path;
//	info.UID = uid;
//	info.localIdentifier = localIdentifier++;

//	json infoData;
//	infoData["UID"] = info.UID;
//	infoData["Path"] = info.path;
//	infoData["localIdentifier"] = info.localIdentifier;

//	std::ofstream infoFile((ProjectManager::GetInstance().GetCurrentProjectPath() + "\\EngineCache\\Meshes\\" +
//meshObj->GetID() + ".mesh.info")); 	if (infoFile.is_open()) { 		infoFile << infoData.dump(4); 		infoFile.close();
//	}
//	AssetRegistry::Instance().RegisterAsset({ meshObj->GetID(), "Mesh", "EngineCache/Meshes/" + meshObj->GetID() +
//".mesh" });
//}
} // namespace REON::EDITOR
