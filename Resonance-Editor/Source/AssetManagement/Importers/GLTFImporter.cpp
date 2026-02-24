#include "GLTFImporter.h"

#include "AssetManagement/CookPipeline.h"

#include <AssetManagement/Assets/Model/TangentCalculator.h>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>

namespace REON::EDITOR
{

bool GltfImporter::CanImport(std::filesystem::path ext) const
{
    return ext == ".gltf" || ext == ".glb";
}

ImportResult GltfImporter::Import(std::filesystem::path src, ImportContext& ctx)
{
    tg::Model model;
    tg::TinyGLTF loader;
    std::string err;
    std::string warn;

    auto extension = src.extension();

    if (extension == ".gltf")
    {
        if (!loader.LoadASCIIFromFile(&model, &err, &warn, src.string()))
        {
            REON_ERROR("Failed to parse GLTF: {}", err);
        }
    }
    else if (extension == ".glb")
    {
        if (!loader.LoadBinaryFromFile(&model, &err, &warn, src.string()))
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

    auto metaPath = src.string() + ".meta";

    currentModelAsset = LoadModelSourceAssetFromFile(metaPath);

    if (currentModelAsset.id == NullAssetId)
    {
        REON_ERROR("GLTF Importer: Source asset missing or invalid ID: {}, stopping import", metaPath);
        return {};
    }

    ImportedModel importedModel{};
    importedModel.modelId = currentModelAsset.id;
    importedModel.sourcePath = currentModelAsset.sourcePath.generic_string();
    importedModel.debugName = currentModelAsset.name;

    AssetRecord modelRecord{};
    modelRecord.id = importedModel.modelId;
    modelRecord.type = ASSET_MODEL;
    modelRecord.sourcePath = importedModel.sourcePath;
    modelRecord.logicalName = currentModelAsset.name;

    materialIDs.clear();

    if (model.extensionsUsed.size() > 0)
        REON_INFO("Model: {} uses extensions:", currentModelAsset.sourcePath.string());
    for (auto khrExtension : model.extensionsUsed)
    {
        REON_INFO("\t{}", khrExtension);
    }

    if (currentModelAsset.importMaterials)
    {
        for (auto& srcMat : model.materials)
        {
            ImportedMaterial mat{};
            mat.id = MakeRandomAssetId();

            mat.debugName = (srcMat.name.empty() ? "Material_" + mat.id.to_string() : srcMat.name);

            AssetRecord matRecord{};
            matRecord.id = mat.id;
            matRecord.logicalName = mat.debugName;
            matRecord.sourcePath = importedModel.sourcePath;
            matRecord.type = ASSET_MATERIAL;

            materialIDs.push_back(mat.id);

            uint32_t flags = 0;

            if (srcMat.alphaMode == "OPAQUE")
            {
                mat.renderingMode = Opaque;
            }
            else if (srcMat.alphaMode == "MASK")
            {
                mat.renderingMode = Transparent;
                mat.blendingMode = Mask;
                flags |= AlphaCutoff;
            }
            else if (srcMat.alphaMode == "BLEND")
            {
                mat.renderingMode = Transparent;
                mat.blendingMode = Blend;
            }
            else
            {
                REON_WARN("Unknown alpha mode, material: {}", srcMat.name);
            }

            mat.emissiveFactor.w = srcMat.alphaCutoff;

            mat.doubleSided = srcMat.doubleSided;

            auto pbrData = srcMat.pbrMetallicRoughness;

            mat.baseColorFactor = glm::vec4(
                static_cast<float>(pbrData.baseColorFactor[0]), static_cast<float>(pbrData.baseColorFactor[1]),
                static_cast<float>(pbrData.baseColorFactor[2]), static_cast<float>(pbrData.baseColorFactor[3]));

            if (pbrData.baseColorTexture.index >= 0)
            {
                mat.baseColorTex =
                    HandleGLTFTexture(model, model.textures[pbrData.baseColorTexture.index], importedModel, ctx);
                matRecord.assetDeps.push_back(mat.baseColorTex);
                flags |= AlbedoTexture;
                if (pbrData.baseColorTexture.texCoord != 0)
                    REON_WARN("Albedo texture has non 0 texcoord and is not used");
            }

            mat.roughness = pbrData.roughnessFactor;
            if (mat.roughness < 0 || mat.roughness > 1)
            {
                mat.roughness = 1.0;
            }
            mat.metallic = pbrData.metallicFactor;
            if (mat.metallic < 0 || mat.metallic > 1)
            {
                mat.metallic = 0.0;
            }

            auto emissiveFactor = srcMat.emissiveFactor;
            mat.emissiveFactor.r = emissiveFactor[0];
            mat.emissiveFactor.g = emissiveFactor[1];
            mat.emissiveFactor.b = emissiveFactor[2];

            for (auto khrExtension : srcMat.extensions)
            {
                if (khrExtension.first == "KHR_materials_emissive_strength")
                {
                    float emissiveStrength = khrExtension.second.Get("emissiveStrength").GetNumberAsDouble();
                    mat.emissiveFactor.r *= emissiveStrength;
                    mat.emissiveFactor.g *= emissiveStrength;
                    mat.emissiveFactor.b *= emissiveStrength;
                }
                else if (khrExtension.first == "KHR_materials_ior")
                {
                    float ior = khrExtension.second.Get("ior").GetNumberAsDouble();
                    mat.precompF0 = pow(((ior - 1.0f) / (ior + 1.0f)), 2.0f);
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
                mat.emissiveTex =
                    HandleGLTFTexture(model, model.textures[srcMat.emissiveTexture.index], importedModel, ctx);
                matRecord.assetDeps.push_back(mat.emissiveTex);
                flags |= EmissiveTexture;
                if (srcMat.emissiveTexture.texCoord != 0)
                    REON_WARN("Emission texture has non 0 texcoord and is not used");
            }

            if (pbrData.metallicRoughnessTexture.index >= 0)
            {
                mat.mrTex = HandleGLTFTexture(model, model.textures[pbrData.metallicRoughnessTexture.index],
                                              importedModel, ctx);
                matRecord.assetDeps.push_back(mat.mrTex);
                flags |= MetallicRoughnessTexture;
                if (pbrData.metallicRoughnessTexture.texCoord != 0)
                    REON_WARN("MetallicRoughness texture has non 0 texcoord and is not used");
            }

            if (srcMat.normalTexture.index >= 0)
            {
                mat.normalTex =
                    HandleGLTFTexture(model, model.textures[srcMat.normalTexture.index], importedModel, ctx);
                matRecord.assetDeps.push_back(mat.normalTex);
                flags |= NormalTexture;
                mat.normalScalar = srcMat.normalTexture.scale;
                if (srcMat.normalTexture.texCoord != 0)
                    REON_WARN("Normal texture has non 0 texcoord and is not used");
            }

            mat.flags = flags;

            importedModel.materials.push_back(mat);
            producedAssets.push_back(matRecord);
        }
    }

    const int sceneToLoad = model.defaultScene < 0 ? 0 : model.defaultScene;

    std::unordered_map<int, std::string> nodeIdtoUuid;

    for (auto& nodeId : model.scenes[sceneToLoad].nodes)
    {
        if (nodeId < 0)
            continue;

        importedModel.rootNodes.push_back(HandleGLTFNode(model, nodeId, importedModel, modelRecord));
    }

    ctx.cache.modelCache[importedModel.modelId] = importedModel;
    producedAssets.push_back(modelRecord);
    return {producedAssets};
}

NodeIndex GltfImporter::HandleGLTFNode(const tg::Model& model, int nodeId, ImportedModel& impModel,
                                       AssetRecord& modelRecord, float scale, uint32_t parentId)
{
    auto node = model.nodes[nodeId];

    ImportedNode data{};
    data.parent = parentId;

    auto trs = GetTRSFromGLTFNode(node);
    data.t = std::get<0>(trs);
    data.r = std::get<1>(trs);
    data.s = std::get<2>(trs) * scale;

    int meshId = node.mesh;

    if (meshId >= 0 && meshId < model.meshes.size())
    {
        data.meshId = HandleGLTFMesh(model, model.meshes[meshId], impModel, data);
        modelRecord.assetDeps.push_back(data.meshId);
    }

    impModel.nodes.push_back(data);
    auto currentId = impModel.nodes.size() - 1; // return index of this node in the imported model's node list

    for (auto& childId : node.children)
    {
        if (childId < 0 || childId > model.nodes.size())
            continue;

        data.children.push_back(HandleGLTFNode(model, childId, impModel, modelRecord, scale, currentId));
    }

    return currentId;
}

AssetId GltfImporter::HandleGLTFMesh(const tg::Model& model, const tg::Mesh& mesh, ImportedModel& impModel,
                                     ImportedNode& impNode)
{
    ImportedMesh meshData{};
    meshData.id = MakeRandomAssetId();
    meshData.debugName = mesh.name;

    AssetRecord meshRecord{};
    meshRecord.id = meshData.id;
    meshRecord.logicalName = meshData.debugName;
    meshRecord.sourcePath = impModel.sourcePath;
    meshRecord.type = ASSET_MESH;

    std::vector<AssetId> matIDsPerMesh;

    int primitiveIndex = 0;
    for (auto primitive : mesh.primitives)
    {
        ImportedMesh::SubMesh subMesh;
        subMesh.indexOffset = meshData.indices.size();

        if (primitive.material >= 0)
        {
            auto matID = materialIDs[primitive.material];
            auto it = std::find(matIDsPerMesh.begin(), matIDsPerMesh.end(), matID);
            if (it != matIDsPerMesh.end())
            {
                subMesh.materialId = std::distance(matIDsPerMesh.begin(), it);
            }
            else
            {
                subMesh.materialId = matIDsPerMesh.size();
                matIDsPerMesh.push_back(matID);
            }
        }
        else
        {
            subMesh.materialId = -1;
        }

        const int vertexOffset = meshData.positions.size();

        for (auto& attribute : primitive.attributes)
        {
            const tinygltf::Accessor& accessor = model.accessors.at(attribute.second);
            if (attribute.first.compare("POSITION") == 0)
            {
                HandleGLTFBuffer(model, accessor, meshData.positions);
            }
            else if (attribute.first.compare("NORMAL") == 0)
            {
                HandleGLTFBuffer(model, accessor, meshData.normals, true);
            }
            else if (attribute.first.compare("TEXCOORD_0") == 0)
            {
                HandleGLTFBuffer(model, accessor, meshData.uv0);
            }
            else if (attribute.first.compare("TANGENT") == 0)
            {
                HandleGLTFBuffer(model, accessor, meshData.tangents);
            }
            else if (attribute.first.compare("COLOR_0") == 0)
            {
                HandleGLTFColor(model, accessor, meshData.colors);
            }
            else
            {
                REON_WARN("More data in GLTF file, accessor name: {}", attribute.first);
            }
        }

        const int smallestBufferSize =
            std::min(meshData.normals.size(), std::min(meshData.uv0.size(), meshData.colors.size()));
        const int vertexCount = meshData.positions.size();
        meshData.normals.reserve(vertexCount);
        meshData.uv0.reserve(vertexCount);
        meshData.colors.reserve(vertexCount);

        for (int i = smallestBufferSize; i < vertexCount; ++i)
        {
            if (meshData.normals.size() == i)
                meshData.normals.push_back(glm::vec3(0, 1, 0));

            if (meshData.uv0.size() == i)
                meshData.uv0.push_back(glm::vec2(0, 0));

            if (meshData.colors.size() == i)
                meshData.colors.push_back(glm::vec4(1, 1, 1, 1));
        }

        const int index = primitive.indices;

        if (index > model.accessors.size())
        {
            REON_WARN("Invalid index accessor index in primitive: {}", std::to_string(primitiveIndex));
            return NullAssetId;
        }

        if (primitive.indices >= 0)
        {
            const tg::Accessor& indexAccessor = model.accessors.at(index);
            HandleGLTFIndices(model, indexAccessor, vertexOffset, meshData.indices);
        }
        else
        {
            for (uint i = static_cast<uint>(vertexOffset); i < meshData.positions.size(); i++)
                meshData.indices.push_back(i);
        }
        subMesh.indexCount = meshData.indices.size() - subMesh.indexOffset;

        primitiveIndex++;

        meshData.subMeshes.push_back(subMesh);
    }

    if (currentModelAsset.generateTangents)
    {
        TangentCalculator::CalculateTangents(meshData.positions, meshData.uv0, meshData.normals, meshData.tangents,
                                             meshData.indices);
    }

    impNode.materialId = matIDsPerMesh;

    impModel.meshes.push_back(meshData);
    producedAssets.push_back(meshRecord);
    return meshData.id;
}

AssetId GltfImporter::HandleGLTFTexture(const tg::Model& model, const tg::Texture& texture, ImportedModel& impModel,
                                        ImportContext& ctx)
{
    const auto& image = model.images[texture.source];

    ImportedTexture importedTexture{};
    ImportedImage importedImage{};

    AssetRecord textureRecord{};

    importedImage.id = MakeRandomAssetId();
    importedImage.debugName = image.name;
    importedImage.width = image.width;
    importedImage.height = image.height;
    importedImage.rgba8 = image.image;
    importedImage.channels = image.component;

    importedTexture.id = MakeRandomAssetId();
    importedTexture.debugName = texture.name;
    importedTexture.imageId = importedImage.id;

    textureRecord.id = importedTexture.id;
    textureRecord.logicalName = importedTexture.debugName;
    textureRecord.sourcePath = impModel.sourcePath;
    textureRecord.type = ASSET_TEXTURE;

    // I know its ugly, but whatever, ill change it later
    if (texture.sampler >= 0)
    {
        const auto& sampler = model.samplers[texture.sampler];

        switch (sampler.magFilter)
        {
        case TINYGLTF_TEXTURE_FILTER_NEAREST:
            importedTexture.samplerStateMagFilter = Nearest;
            break;
        case TINYGLTF_TEXTURE_FILTER_LINEAR:
            importedTexture.samplerStateMagFilter = Linear;
            break;
        default:
            REON_WARN("Unknown magfilter on texture {}", sampler.magFilter);
            break;
        }
        switch (sampler.minFilter)
        {
        case TINYGLTF_TEXTURE_FILTER_NEAREST:
            importedTexture.samplerStateMinFilter = Nearest;
            break;
        case TINYGLTF_TEXTURE_FILTER_LINEAR:
            importedTexture.samplerStateMinFilter = Linear;
            break;
        case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
            importedTexture.samplerStateMinFilter = Nearest;
            break;
        case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
            importedTexture.samplerStateMinFilter = Nearest;
            break;
        case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
            importedTexture.samplerStateMinFilter = Linear;
            break;
        case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
            importedTexture.samplerStateMinFilter = Linear;
            break;
        default:
            REON_WARN("Unknown minFilter on texture {}", sampler.minFilter);
            break;
        }
        switch (sampler.wrapS)
        {
        case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
            importedTexture.samplerStateWrapU = ClampToEdge;
            break;
        case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
            importedTexture.samplerStateWrapU = MirroredRepeat;
            break;
        case TINYGLTF_TEXTURE_WRAP_REPEAT:
            importedTexture.samplerStateWrapU = Repeat;
            break;
        default:
            REON_WARN("Unknown wrap U on texture {}", sampler.wrapS);
            break;
        }
        switch (sampler.wrapT)
        {
        case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
            importedTexture.samplerStateWrapV = ClampToEdge;
            break;
        case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
            importedTexture.samplerStateWrapV = MirroredRepeat;
            break;
        case TINYGLTF_TEXTURE_WRAP_REPEAT:
            importedTexture.samplerStateWrapV = Repeat;
            break;
        default:
            REON_WARN("Unknown wrap V on texture {}", sampler.wrapT);
            break;
        }
    }
    // std::ofstream out(fullPath.parent_path().string() + "\\" + path + ".img", std::ios::binary);
    // if (out.is_open())
    //{
    //     out.write(reinterpret_cast<const char*>(&header), sizeof(TextureHeader));
    //     out.write(reinterpret_cast<const char*>(image.image.data()), image.image.size() * sizeof(image.image[0]));
    //     out.close();
    // }
    impModel.images.push_back(importedImage);
    impModel.textures.push_back(importedTexture);
    producedAssets.push_back(textureRecord);
    return importedTexture.id;
    // ctx.cache.textureCache[importedTexture.id] = importedTexture;
}

void GltfImporter::HandleGLTFBuffer(const tg::Model& model, const tg::Accessor& accessor, std::vector<glm::vec3>& data,
                                    bool normal)
{
    if (accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT)
    {
        REON_WARN("Only float component type is supported for vec3 buffers, accessor name: {}", accessor.name);
        return;
    }

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
void GltfImporter::HandleGLTFBuffer(const tinygltf::Model& model, const tinygltf::Accessor& accessor,
                                    std::vector<T>& data)
{
    if (accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT)
    {
        REON_WARN("Only float component type is supported for template buffers at the moment, accessor name: {}",
                  accessor.name);
        return;
    }

    const tg::BufferView& bufferView = model.bufferViews.at(accessor.bufferView);
    const tg::Buffer& buffer = model.buffers.at(bufferView.buffer);
    const int bufferStart = bufferView.byteOffset + accessor.byteOffset;
    const int stride = accessor.ByteStride(bufferView);
    const int bufferEnd = bufferStart + accessor.count * stride;

    const int dataStart = data.size();
    data.reserve(dataStart + accessor.count);

    for (size_t i = bufferStart; i < bufferEnd; i += stride)
    {
        T v;
        std::memcpy(&v, &buffer.data[i], sizeof(T));
        data.push_back(v);
    }

    if (accessor.sparse.isSparse)
        REON_WARN("Sparse accessors are not supported yet (while processing template buffer)");
}

float GltfImporter::ReadNormalizedComponent(const uint8_t* p, int componentType, bool normalized)
{
    switch (componentType)
    {
    case TINYGLTF_COMPONENT_TYPE_FLOAT:
    {
        float v;
        std::memcpy(&v, p, sizeof(float));
        return v;
    }
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
    {
        uint8_t v = *p;
        return normalized ? (float(v) / 255.0f) : float(v);
    }
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
    {
        uint16_t v;
        std::memcpy(&v, p, sizeof(uint16_t));
        return normalized ? (float(v) / 65535.0f) : float(v);
    }
    // glTF also allows BYTE/SHORT; add if you want:
    case TINYGLTF_COMPONENT_TYPE_BYTE:
    {
        int8_t v;
        std::memcpy(&v, p, sizeof(int8_t));
        return normalized ? std::max(-1.0f, float(v) / 127.0f) : float(v);
    }
    case TINYGLTF_COMPONENT_TYPE_SHORT:
    {
        int16_t v;
        std::memcpy(&v, p, sizeof(int16_t));
        return normalized ? std::max(-1.0f, float(v) / 32767.0f) : float(v);
    }
    default:
        return std::numeric_limits<float>::quiet_NaN();
    }
}

void GltfImporter::HandleGLTFColor(const tinygltf::Model& model, const tinygltf::Accessor& accessor,
                                       std::vector<glm::vec4>& out)
{
    const auto& bufferView = model.bufferViews.at(accessor.bufferView);
    const auto& buffer = model.buffers.at(bufferView.buffer);

    const size_t base = size_t(bufferView.byteOffset) + size_t(accessor.byteOffset);

    int stride = accessor.ByteStride(bufferView);
    if (stride == 0)
    {
        stride =
            tinygltf::GetNumComponentsInType(accessor.type) * tinygltf::GetComponentSizeInBytes(accessor.componentType);
    }

    const int comps = tinygltf::GetNumComponentsInType(accessor.type);
    if (accessor.type != TINYGLTF_TYPE_VEC3 && accessor.type != TINYGLTF_TYPE_VEC4)
    {
        REON_ERROR("Accessor type {} is not supported for color buffer, only vec3 and vec4 are supported",
                   accessor.type);
        return;
    }

    out.reserve(out.size() + accessor.count);

    for (size_t idx = 0; idx < accessor.count; ++idx)
    {
        const uint8_t* p = buffer.data.data() + base + idx * size_t(stride);

        float r = ReadNormalizedComponent(p + 0 * tinygltf::GetComponentSizeInBytes(accessor.componentType),
                                          accessor.componentType, accessor.normalized);
        float g = ReadNormalizedComponent(p + 1 * tinygltf::GetComponentSizeInBytes(accessor.componentType),
                                          accessor.componentType, accessor.normalized);
        float b = ReadNormalizedComponent(p + 2 * tinygltf::GetComponentSizeInBytes(accessor.componentType),
                                          accessor.componentType, accessor.normalized);

        float a = 1.0f;
        if (comps == 4)
        {
            a = ReadNormalizedComponent(p + 3 * tinygltf::GetComponentSizeInBytes(accessor.componentType),
                                        accessor.componentType, accessor.normalized);
        }

        out.emplace_back(r, g, b, a);
    }

    if (accessor.sparse.isSparse)
        REON_WARN("Sparse accessors are not supported yet (while processing color buffer)");
}

void GltfImporter::HandleGLTFIndices(const tg::Model& model, const tg::Accessor& accessor, int offset,
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

std::tuple<glm::vec3, Quaternion, glm::vec3> GltfImporter::GetTRSFromGLTFNode(const tg::Node& node)
{
    glm::vec3 trans{0, 0, 0};
    Quaternion rot{1, 0, 0, 0};
    glm::vec3 scale{1, 1, 1};
    if (node.matrix.size() == 16)
    {
        glm::mat4 matrix(static_cast<float>(node.matrix[0]), static_cast<float>(node.matrix[1]),
                         static_cast<float>(node.matrix[2]), static_cast<float>(node.matrix[3]),
                         static_cast<float>(node.matrix[4]), static_cast<float>(node.matrix[5]),
                         static_cast<float>(node.matrix[6]), static_cast<float>(node.matrix[7]),
                         static_cast<float>(node.matrix[8]), static_cast<float>(node.matrix[9]),
                         static_cast<float>(node.matrix[10]), static_cast<float>(node.matrix[11]),
                         static_cast<float>(node.matrix[12]), static_cast<float>(node.matrix[13]),
                         static_cast<float>(node.matrix[14]), static_cast<float>(node.matrix[15]));
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(matrix, scale, rot, trans, skew, perspective);
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
static bool registered = []()
{
    CookPipeline::RegisterImporter(ASSET_MODEL, std::make_unique<GltfImporter>());
    return true;
}();
} // namespace REON::EDITOR