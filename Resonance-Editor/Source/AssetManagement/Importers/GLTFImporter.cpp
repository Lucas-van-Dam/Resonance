#define TINYGLTF_IMPLEMENTATION
#include "GLTFImporter.h"

#include "AssetManagement/CookPipeline.h"

#include <AssetManagement/Assets/Model/TangentCalculator.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>
#include <AssetManagement/ImportedSourceStore.h>

namespace REON_EDITOR
{

bool GltfImporter::CanImport(std::filesystem::path ext) const
{
    return ext == ".gltf" || ext == ".glb";
}

static void getTexAndImageId(REON::AssetId& texId, REON::AssetId& imgId, ModelSourceAsset& asset, int& currentId)
{
    if (asset.textureIds.size() > currentId)
    {
        texId = asset.textureIds[currentId];
    }
    else
    {
        texId = REON::MakeRandomAssetId();
        asset.textureIds.push_back(texId);
    }
    if (asset.imageIds.size() > currentId)
    {
        imgId = asset.imageIds[currentId];
    }
    else
    {
        imgId = REON::MakeRandomAssetId();
        asset.imageIds.push_back(imgId);
    }
    currentId++;
}

ImportResult GltfImporter::Import(std::filesystem::path src)
{
    producedAssets.clear();
    currentMeshId = 0;

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

    if (currentModelAsset.id == REON::NullAssetId)
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
    modelRecord.type = REON::ASSET_MODEL;
    modelRecord.sourcePath = importedModel.sourcePath;
    modelRecord.logicalName = currentModelAsset.name;
    modelRecord.origin = AssetOrigin::ImportedRootAsset;

    materialIDs.clear();

    if (model.extensionsUsed.size() > 0)
        REON_INFO("Model: {} uses extensions:", currentModelAsset.sourcePath.string());
    for (auto khrExtension : model.extensionsUsed)
    {
        REON_INFO("\t{}", khrExtension);
    }

    int currentId = 0;

    if (currentModelAsset.importMaterials)
    {
        bool ReassignIds = model.materials.size() != currentModelAsset.materialIds.size();
        for (int i = 0; i < model.materials.size(); ++i)
        {
            auto& srcMat = model.materials[i];
            ImportedMaterial mat{};
            if (ReassignIds)
                mat.id = REON::MakeRandomAssetId();
            else
                mat.id = currentModelAsset.materialIds[i];

            mat.debugName = (srcMat.name.empty() ? "Material_" + mat.id.to_string() : srcMat.name);

            AssetRecord matRecord{};
            matRecord.id = mat.id;
            matRecord.logicalName = mat.debugName;
            matRecord.sourcePath = importedModel.sourcePath;
            matRecord.type = REON::ASSET_MATERIAL;
            matRecord.parentSourceId = importedModel.modelId;
            matRecord.origin = AssetOrigin::ImportedSubAsset;

            materialIDs.push_back(mat.id);

            uint32_t flags = 0;

            if (srcMat.alphaMode == "OPAQUE")
            {
                mat.renderingMode = REON::Opaque;
            }
            else if (srcMat.alphaMode == "MASK")
            {
                mat.renderingMode = REON::Transparent;
                mat.blendingMode = REON::Mask;
                flags |= REON::AlphaCutoff;
            }
            else if (srcMat.alphaMode == "BLEND")
            {
                mat.renderingMode = REON::Transparent;
                mat.blendingMode = REON::Blend;
            }
            else
            {
                REON_WARN("Unknown alpha mode, material: {}", srcMat.name);
            }

            mat.emissiveFactor.w = srcMat.alphaCutoff;

            mat.doubleSided = srcMat.doubleSided;
            mat.flipNormals = currentModelAsset.flipNormals;

            auto pbrData = srcMat.pbrMetallicRoughness;

            mat.baseColorFactor = glm::vec4(
                static_cast<float>(pbrData.baseColorFactor[0]), static_cast<float>(pbrData.baseColorFactor[1]),
                static_cast<float>(pbrData.baseColorFactor[2]), static_cast<float>(pbrData.baseColorFactor[3]));

            if (pbrData.baseColorTexture.index >= 0)
            {
                REON::AssetId imgId;
                REON::AssetId texId;
                getTexAndImageId(texId, imgId, currentModelAsset, currentId);
                mat.baseColorTex = HandleGLTFTexture(model, model.textures[pbrData.baseColorTexture.index],
                                                     importedModel, true, texId, imgId);
                matRecord.assetDeps.push_back(mat.baseColorTex);
                flags |= REON::AlbedoTexture;
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
                REON::AssetId imgId;
                REON::AssetId texId;
                getTexAndImageId(texId, imgId, currentModelAsset, currentId);
                mat.emissiveTex = HandleGLTFTexture(model, model.textures[srcMat.emissiveTexture.index], importedModel,
                                                    true, texId, imgId);
                matRecord.assetDeps.push_back(mat.emissiveTex);
                flags |= REON::EmissiveTexture;
                if (srcMat.emissiveTexture.texCoord != 0)
                    REON_WARN("Emission texture has non 0 texcoord and is not used");
            }

            if (pbrData.metallicRoughnessTexture.index >= 0)
            {
                REON::AssetId imgId;
                REON::AssetId texId;
                getTexAndImageId(texId, imgId, currentModelAsset, currentId);
                mat.mrTex = HandleGLTFTexture(model, model.textures[pbrData.metallicRoughnessTexture.index],
                                              importedModel, false, texId, imgId);
                matRecord.assetDeps.push_back(mat.mrTex);
                flags |= REON::MetallicRoughnessTexture;
                if (pbrData.metallicRoughnessTexture.texCoord != 0)
                    REON_WARN("MetallicRoughness texture has non 0 texcoord and is not used");
            }

            if (srcMat.normalTexture.index >= 0)
            {
                REON::AssetId imgId;
                REON::AssetId texId;
                getTexAndImageId(texId, imgId, currentModelAsset, currentId);
                mat.normalTex = HandleGLTFTexture(model, model.textures[srcMat.normalTexture.index], importedModel,
                                                  false, texId, imgId);
                matRecord.assetDeps.push_back(mat.normalTex);
                flags |= REON::NormalTexture;
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

    importedModel.nodes.resize(model.nodes.size());

    for (auto& nodeId : model.scenes[sceneToLoad].nodes)
    {
        if (nodeId < 0)
            continue;

        importedModel.rootNodes.push_back(HandleGLTFNode(model, nodeId, importedModel, modelRecord));
    }

    if (!model.skins.empty())
    {
        importedModel.rig = ImportedRig{};
        importedModel.rig.value().rigId = REON::MakeRandomAssetId();
    }

    std::vector<uint32_t> rigJointNodes;
    rigJointNodes.reserve(256);

    auto add_unique = [&](uint32_t nodeIdx)
    {
        if (std::find(rigJointNodes.begin(), rigJointNodes.end(), nodeIdx) == rigJointNodes.end())
            rigJointNodes.push_back(nodeIdx);
    };

    for (const auto& skin : model.skins)
    {
        for (int i = 0; i < (int)skin.joints.size(); ++i)
            add_unique((uint32_t)skin.joints[i]);
    }

    std::unordered_map<uint32_t, uint32_t> nodeToPalette;
    nodeToPalette.reserve(rigJointNodes.size());
    for (uint32_t i = 0; i < (uint32_t)rigJointNodes.size(); ++i)
        nodeToPalette[rigJointNodes[i]] = i;

    if (importedModel.rig.has_value())
    {
        importedModel.rig->joints.resize(rigJointNodes.size());
        for (uint32_t i = 0; i < (uint32_t)rigJointNodes.size(); ++i)
        {
            const uint32_t nodeIdx = rigJointNodes[i];

            importedModel.rig->joints[i] = importedModel.nodes[nodeIdx].NodeId;
        }
    }

    for (const auto& skin : model.skins)
    {
        ImportedSkin impSkin;

        if (skin.inverseBindMatrices >= 0)
        {
            const auto& accessor = model.accessors.at(skin.inverseBindMatrices);
            ReadAccessorMat4(model, accessor, impSkin.inverseBindMatrices);
        }

        impSkin.jointIndices.resize(skin.joints.size());
        for (int i = 0; i < (int)skin.joints.size(); ++i)
        {
            const uint32_t nodeIdx = (uint32_t)skin.joints[i];
            auto it = nodeToPalette.find(nodeIdx);
            if (it == nodeToPalette.end())
            {
                REON_ERROR("Skin joint node {} missing from rig palette", nodeIdx);
                impSkin.jointIndices[i] = 0;
            }
            else
            {
                impSkin.jointIndices[i] = it->second;
            }
        }

        if (skin.skeleton >= 0)
        {
            const uint32_t skelNode = (uint32_t)skin.skeleton;
            auto it = nodeToPalette.find(skelNode);
            if (it != nodeToPalette.end())
                impSkin.skeleton = it->second;
            else
                impSkin.skeleton = UINT32_MAX;
        }
        else
        {
            impSkin.skeleton = UINT32_MAX;
        }

        importedModel.skins.push_back(std::move(impSkin));
    }

    //TODO: persist import data on disk
    producedAssets.push_back(modelRecord);

    auto lastWriteTime = std::filesystem::last_write_time(metaPath);
    auto buildKey = std::chrono::duration_cast<std::chrono::milliseconds>(lastWriteTime.time_since_epoch()).count();
    currentModelAsset.lastBuildKey = buildKey;

    if (currentModelAsset.materialIds.size() != importedModel.materials.size())
    {
        for (auto& mat : importedModel.materials)
        {
            currentModelAsset.materialIds.push_back(mat.id);
        }
    }

    SaveModelSourceAssetToFile(metaPath, currentModelAsset);

    ImportedSourceStore::SaveModel(importedModel.modelId, importedModel);

    return {producedAssets};
}

NodeIndex GltfImporter::HandleGLTFNode(const tg::Model& model, int nodeId, ImportedModel& impModel,
                                       AssetRecord& modelRecord, float scale, uint32_t parentId)
{
    auto node = model.nodes[nodeId];

    ImportedNode data{};
    data.parent = parentId;

    data.debugName = node.name;

    data.NodeId = REON::MakeRandomAssetId();

    auto trs = GetTRSFromGLTFNode(node);
    data.t = std::get<0>(trs);
    data.r = std::get<1>(trs);
    data.s = std::get<2>(trs) * scale;

    int meshId = node.mesh;

    if (meshId >= 0 && meshId < model.meshes.size())
    {
        REON::AssetId id;
        if (currentModelAsset.meshIds.size() > currentMeshId)
            id = currentModelAsset.meshIds[currentMeshId];
        else
        {
            id = REON::MakeRandomAssetId();
            currentModelAsset.meshIds.push_back(id);
        }
        data.meshId = HandleGLTFMesh(model, model.meshes[meshId], impModel, data, id);
        modelRecord.assetDeps.push_back(data.meshId);
    }

    if (node.skin >= 0)
    {
        REON_CORE_INFO("Mesh has skin");
        data.skinIndex = node.skin;
    }
    else
    {
        data.skinIndex = UINT32_MAX;
    }

    impModel.nodes[nodeId] = data;
    auto currentId = impModel.nodes.size() - 1; // return index of this node in the imported model's node list

    for (auto& childId : node.children)
    {
        if (childId < 0 || childId > model.nodes.size() - 1)
            continue;

        const auto importedChildId = HandleGLTFNode(model, childId, impModel, modelRecord, scale, currentId);

        impModel.nodes[nodeId].children.push_back(importedChildId);
    }

    return nodeId;
}

REON::AssetId GltfImporter::HandleGLTFMesh(const tg::Model& model, const tg::Mesh& mesh, ImportedModel& impModel,
                                           ImportedNode& impNode, REON::AssetId id)
{
    ImportedMesh meshData{};
    meshData.id = id;
    meshData.debugName = mesh.name;

    AssetRecord meshRecord{};
    meshRecord.id = meshData.id;
    meshRecord.logicalName = meshData.debugName;
    meshRecord.sourcePath = impModel.sourcePath;
    meshRecord.type = REON::ASSET_MESH;
    meshRecord.origin = AssetOrigin::ImportedSubAsset;
    meshRecord.parentSourceId = impModel.modelId;

    std::vector<REON::AssetId> matIDsPerMesh;

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
                ReadAccessorVec3(model, accessor, meshData.positions);
                if (currentModelAsset.scale != 1.0f)
                {
                    for (auto& pos : meshData.positions)
                    {
                        pos *= currentModelAsset.scale;
                    }
                }
            }
            else if (attribute.first.compare("NORMAL") == 0)
            {
                ReadAccessorVec3(model, accessor, meshData.normals);
            }
            else if (attribute.first.compare("TEXCOORD_0") == 0)
            {
                ReadAccessorVec2(model, accessor, meshData.uv0);
            }
            else if (attribute.first.compare("TANGENT") == 0)
            {
                ReadAccessorVec4(model, accessor, meshData.tangents);
            }
            else if (attribute.first.compare("COLOR_0") == 0)
            {
                ReadAccessorColor(model, accessor, meshData.colors);
            }
            else if (attribute.first.compare("JOINTS_0") == 0)
            {
                ReadAccessorJoints(model, accessor, meshData.joints_0);
            }
            else if (attribute.first.compare("WEIGHTS_0") == 0)
            {
                ReadAccessorWeights(model, accessor, meshData.weights_0);
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
            return REON::NullAssetId;
        }

        if (primitive.indices >= 0)
        {
            const tg::Accessor& indexAccessor = model.accessors.at(index);
            ReadAccessorIndices(model, indexAccessor, meshData.indices, vertexOffset);
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

    if (meshData.tangents.empty() || currentModelAsset.forceGenerateTangents)
    {
        TangentCalculator::CalculateTangents(meshData.positions, meshData.uv0, meshData.normals, meshData.tangents,
                                             meshData.indices);
    }

    impNode.materialId = matIDsPerMesh;

    impModel.meshes.push_back(meshData);
    producedAssets.push_back(meshRecord);
    return meshData.id;
}

REON::AssetId GltfImporter::HandleGLTFTexture(const tg::Model& model, const tg::Texture& texture,
                                              ImportedModel& impModel, bool isSRGB, REON::AssetId texId,
                                              REON::AssetId imgId)
{
    const auto& image = model.images[texture.source];

    ImportedTexture importedTexture{};
    ImportedImage importedImage{};

    AssetRecord textureRecord{};
    importedImage.id = imgId;
    importedImage.debugName = image.name;
    importedImage.width = image.width;
    importedImage.height = image.height;
    importedImage.rgba8 = image.image;
    importedImage.channels = image.component;
    importedImage.srgb = isSRGB;

    importedTexture.id = texId;
    importedTexture.debugName = texture.name;
    importedTexture.imageId = importedImage.id;

    textureRecord.id = importedTexture.id;
    textureRecord.logicalName = importedTexture.debugName;
    textureRecord.sourcePath = impModel.sourcePath;
    textureRecord.type = REON::ASSET_TEXTURE;
    textureRecord.origin = ImportedSubAsset;
    textureRecord.parentSourceId = impModel.modelId;

    // I know its ugly, but whatever, ill change it later
    if (texture.sampler >= 0)
    {
        const auto& sampler = model.samplers[texture.sampler];

        switch (sampler.magFilter)
        {
        case TINYGLTF_TEXTURE_FILTER_NEAREST:
            importedTexture.samplerStateMagFilter = REON::Nearest;
            break;
        case TINYGLTF_TEXTURE_FILTER_LINEAR:
            importedTexture.samplerStateMagFilter = REON::Linear;
            break;
        default:
            REON_WARN("Unknown magfilter on texture {}", sampler.magFilter);
            break;
        }
        switch (sampler.minFilter)
        {
        case TINYGLTF_TEXTURE_FILTER_NEAREST:
            importedTexture.samplerStateMinFilter = REON::Nearest;
            break;
        case TINYGLTF_TEXTURE_FILTER_LINEAR:
            importedTexture.samplerStateMinFilter = REON::Linear;
            break;
        case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
            importedTexture.samplerStateMinFilter = REON::Nearest;
            break;
        case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
            importedTexture.samplerStateMinFilter = REON::Nearest;
            break;
        case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
            importedTexture.samplerStateMinFilter = REON::Linear;
            break;
        case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
            importedTexture.samplerStateMinFilter = REON::Linear;
            break;
        default:
            REON_WARN("Unknown minFilter on texture {}", sampler.minFilter);
            break;
        }
        switch (sampler.wrapS)
        {
        case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
            importedTexture.samplerStateWrapU = REON::ClampToEdge;
            break;
        case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
            importedTexture.samplerStateWrapU = REON::MirroredRepeat;
            break;
        case TINYGLTF_TEXTURE_WRAP_REPEAT:
            importedTexture.samplerStateWrapU = REON::Repeat;
            break;
        default:
            REON_WARN("Unknown wrap U on texture {}", sampler.wrapS);
            break;
        }
        switch (sampler.wrapT)
        {
        case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
            importedTexture.samplerStateWrapV = REON::ClampToEdge;
            break;
        case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
            importedTexture.samplerStateWrapV = REON::MirroredRepeat;
            break;
        case TINYGLTF_TEXTURE_WRAP_REPEAT:
            importedTexture.samplerStateWrapV = REON::Repeat;
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
}

std::tuple<glm::vec3, REON::Quaternion, glm::vec3> GltfImporter::GetTRSFromGLTFNode(const tg::Node& node)
{
    glm::vec3 trans{0, 0, 0};
    REON::Quaternion rot{1, 0, 0, 0};
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
        rot = glm::normalize(rot);

        return {trans, rot, scale};
    }
    else
    {
        if (node.scale.size() == 3)
            scale = glm::vec3(static_cast<float>(node.scale[0]), static_cast<float>(node.scale[1]),
                              static_cast<float>(node.scale[2]));

        if (node.rotation.size() == 4)
            rot = REON::Quaternion(static_cast<float>(node.rotation[3]), static_cast<float>(node.rotation[0]),
                             static_cast<float>(node.rotation[1]), static_cast<float>(node.rotation[2]));
        rot = glm::normalize(rot);

        if (node.translation.size() == 3)
            trans = glm::vec3(static_cast<float>(node.translation[0]), static_cast<float>(node.translation[1]),
                              static_cast<float>(node.translation[2]));

        return {trans, rot, scale};
    }
}

static inline size_t ComponentSizeBytes(int componentType)
{
    return size_t(tg::GetComponentSizeInBytes(componentType));
}

static inline int NumComps(int accessorType)
{
    return tg::GetNumComponentsInType(accessorType);
}

void GltfImporter::GetAccessorBaseAndStride(const tg::Model& model, const tg::Accessor& accessor, const uint8_t*& base,
                                            size_t& strideBytes)
{
    const auto& bv = model.bufferViews.at(accessor.bufferView);
    const auto& buf = model.buffers.at(bv.buffer);

    base = buf.data.data() + size_t(bv.byteOffset) + size_t(accessor.byteOffset);

    strideBytes = size_t(accessor.ByteStride(bv));
    if (strideBytes == 0)
        strideBytes = size_t(NumComps(accessor.type)) * ComponentSizeBytes(accessor.componentType);
}

float GltfImporter::ReadComponentAsFloat(const uint8_t* p, int componentType, bool normalized)
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
    case TINYGLTF_COMPONENT_TYPE_BYTE:
    {
        int8_t v;
        std::memcpy(&v, p, sizeof(int8_t));
        if (!normalized)
            return float(v);
        // glTF: normalized signed maps to [-1, 1]
        // clamp lower bound to -1 (because -128/127 < -1)
        float f = float(v) / 127.0f;
        return (f < -1.0f) ? -1.0f : f;
    }
    case TINYGLTF_COMPONENT_TYPE_SHORT:
    {
        int16_t v;
        std::memcpy(&v, p, sizeof(int16_t));
        if (!normalized)
            return float(v);
        float f = float(v) / 32767.0f;
        return (f < -1.0f) ? -1.0f : f;
    }
    default:
        return std::numeric_limits<float>::quiet_NaN();
    }
}

uint32_t GltfImporter::ReadComponentAsU32(const uint8_t* p, int componentType)
{
    switch (componentType)
    {
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
        return uint32_t(*p);
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
    {
        uint16_t v;
        std::memcpy(&v, p, sizeof(uint16_t));
        return uint32_t(v);
    }
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
    {
        uint32_t v;
        std::memcpy(&v, p, sizeof(uint32_t));
        return v;
    }
    default:
        return 0;
    }
}

glm::vec2 GltfImporter::ReadVec2(const tg::Accessor& accessor, const uint8_t* p)
{
    glm::vec2 v;
    const size_t cs = ComponentSizeBytes(accessor.componentType);
    v.x = ReadComponentAsFloat(p + 0 * cs, accessor.componentType, accessor.normalized);
    v.y = ReadComponentAsFloat(p + 1 * cs, accessor.componentType, accessor.normalized);
    return v;
}

glm::vec3 GltfImporter::ReadVec3(const tg::Accessor& accessor, const uint8_t* p)
{
    glm::vec3 v;
    const size_t cs = ComponentSizeBytes(accessor.componentType);
    v.x = ReadComponentAsFloat(p + 0 * cs, accessor.componentType, accessor.normalized);
    v.y = ReadComponentAsFloat(p + 1 * cs, accessor.componentType, accessor.normalized);
    v.z = ReadComponentAsFloat(p + 2 * cs, accessor.componentType, accessor.normalized);
    return v;
}

glm::vec4 GltfImporter::ReadVec4(const tg::Accessor& accessor, const uint8_t* p)
{
    glm::vec4 v;
    const size_t cs = ComponentSizeBytes(accessor.componentType);
    v.x = ReadComponentAsFloat(p + 0 * cs, accessor.componentType, accessor.normalized);
    v.y = ReadComponentAsFloat(p + 1 * cs, accessor.componentType, accessor.normalized);
    v.z = ReadComponentAsFloat(p + 2 * cs, accessor.componentType, accessor.normalized);
    v.w = ReadComponentAsFloat(p + 3 * cs, accessor.componentType, accessor.normalized);
    return v;
}

glm::mat4 GltfImporter::ReadMat4(const tg::Accessor& accessor, const uint8_t* p)
{
    glm::mat4 m(1.0f);

    if (accessor.type != TINYGLTF_TYPE_MAT4)
        return m;

    // glTF matrices are stored column-major (same convention as GLM default).
    // Each MAT4 is 16 scalars laid out as: m00,m10,m20,m30, m01,m11,... (i.e., columns contiguous).
    const size_t cs = ComponentSizeBytes(accessor.componentType);

    // Fast path: common case for inverseBindMatrices is FLOAT, not normalized
    if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT && !accessor.normalized)
    {
        static_assert(sizeof(glm::mat4) == sizeof(float) * 16, "Unexpected glm::mat4 layout");
        std::memcpy(glm::value_ptr(m), p, sizeof(float) * 16);
        return m;
    }

    // Generic path (handles non-float, normalized, etc.)
    for (int col = 0; col < 4; ++col)
    {
        for (int row = 0; row < 4; ++row)
        {
            const size_t idx = size_t(col * 4 + row); // column-major indexing in glTF
            m[col][row] = ReadComponentAsFloat(p + idx * cs, accessor.componentType, accessor.normalized);
        }
    }

    return m;
}

glm::u16vec4 GltfImporter::ReadJointsU16x4(const tg::Accessor& accessor, const uint8_t* p)
{
    glm::u16vec4 out(0);

    // glTF allows JOINTS_0 componentType: UNSIGNED_BYTE or UNSIGNED_SHORT
    if (accessor.type != TINYGLTF_TYPE_VEC4)
        return out;
    if (accessor.normalized)
        return out;

    if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
    {
        out.x = uint16_t(p[0]);
        out.y = uint16_t(p[1]);
        out.z = uint16_t(p[2]);
        out.w = uint16_t(p[3]);
        return out;
    }
    if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
    {
        uint16_t v[4];
        std::memcpy(v, p, sizeof(v));
        out = glm::u16vec4(v[0], v[1], v[2], v[3]);
        return out;
    }

    return out;
}

glm::vec4 GltfImporter::ReadWeightsVec4(const tg::Accessor& accessor, const uint8_t* p)
{
    if (accessor.type != TINYGLTF_TYPE_VEC4)
        return glm::vec4(0.0f);

    const size_t cs = ComponentSizeBytes(accessor.componentType);

    glm::vec4 w;
    w.x = ReadComponentAsFloat(p + 0 * cs, accessor.componentType, accessor.normalized);
    w.y = ReadComponentAsFloat(p + 1 * cs, accessor.componentType, accessor.normalized);
    w.z = ReadComponentAsFloat(p + 2 * cs, accessor.componentType, accessor.normalized);
    w.w = ReadComponentAsFloat(p + 3 * cs, accessor.componentType, accessor.normalized);
    return w;
}

bool GltfImporter::ReadAccessorVec2(const tg::Model& model, const tg::Accessor& accessor, std::vector<glm::vec2>& out)
{
    if (accessor.type != TINYGLTF_TYPE_VEC2)
        return false;

    const uint8_t* base = nullptr;
    size_t stride = 0;
    GetAccessorBaseAndStride(model, accessor, base, stride);

    out.reserve(out.size() + size_t(accessor.count));

    for (size_t i = 0; i < size_t(accessor.count); ++i)
        out.push_back(ReadVec2(accessor, base + i * stride));

    return true;
}

bool GltfImporter::ReadAccessorVec3(const tg::Model& model, const tg::Accessor& accessor, std::vector<glm::vec3>& out)
{
    if (accessor.type != TINYGLTF_TYPE_VEC3)
        return false;

    const uint8_t* base = nullptr;
    size_t stride = 0;
    GetAccessorBaseAndStride(model, accessor, base, stride);

    out.reserve(out.size() + size_t(accessor.count));

    for (size_t i = 0; i < size_t(accessor.count); ++i)
        out.push_back(ReadVec3(accessor, base + i * stride));

    return true;
}

bool GltfImporter::ReadAccessorVec4(const tg::Model& model, const tg::Accessor& accessor, std::vector<glm::vec4>& out)
{
    if (accessor.type != TINYGLTF_TYPE_VEC4)
        return false;

    const uint8_t* base = nullptr;
    size_t stride = 0;
    GetAccessorBaseAndStride(model, accessor, base, stride);

    out.reserve(out.size() + size_t(accessor.count));

    for (size_t i = 0; i < size_t(accessor.count); ++i)
        out.push_back(ReadVec4(accessor, base + i * stride));

    return true;
}

bool GltfImporter::ReadAccessorMat4(const tg::Model& model, const tg::Accessor& accessor, std::vector<glm::mat4>& out)
{
    if (accessor.type != TINYGLTF_TYPE_MAT4)
        return false;

    const uint8_t* base = nullptr;
    size_t stride = 0;
    GetAccessorBaseAndStride(model, accessor, base, stride);

    // If ByteStride() returns 0, your helper sets it to numComps * compSize.
    // For MAT4, NumComps(type) should be 16.
    out.reserve(out.size() + size_t(accessor.count));

    for (size_t i = 0; i < size_t(accessor.count); ++i)
        out.push_back(ReadMat4(accessor, base + i * stride));

    return true;
}

bool GltfImporter::ReadAccessorColor(const tg::Model& model, const tg::Accessor& accessor, std::vector<glm::vec4>& out)
{
    if (accessor.type != TINYGLTF_TYPE_VEC3 && accessor.type != TINYGLTF_TYPE_VEC4)
        return false;

    const uint8_t* base = nullptr;
    size_t stride = 0;
    GetAccessorBaseAndStride(model, accessor, base, stride);

    out.reserve(out.size() + size_t(accessor.count));

    const int comps = NumComps(accessor.type);
    for (size_t i = 0; i < size_t(accessor.count); ++i)
    {
        const uint8_t* p = base + i * stride;
        glm::vec4 c = (comps == 3) ? glm::vec4(ReadVec3(accessor, p), 1.0f) : ReadVec4(accessor, p);
        out.push_back(c);
    }
    return true;
}

bool GltfImporter::ReadAccessorJoints(const tg::Model& model, const tg::Accessor& accessor,
                                      std::vector<glm::u16vec4>& out)
{
    if (accessor.type != TINYGLTF_TYPE_VEC4)
        return false;
    if (accessor.componentType != TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE &&
        accessor.componentType != TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
        return false;

    const uint8_t* base = nullptr;
    size_t stride = 0;
    GetAccessorBaseAndStride(model, accessor, base, stride);

    out.reserve(out.size() + size_t(accessor.count));

    for (size_t i = 0; i < size_t(accessor.count); ++i)
        out.push_back(ReadJointsU16x4(accessor, base + i * stride));

    return true;
}

bool GltfImporter::ReadAccessorWeights(const tg::Model& model, const tg::Accessor& accessor,
                                       std::vector<glm::vec4>& out)
{
    if (accessor.type != TINYGLTF_TYPE_VEC4)
        return false;

    const uint8_t* base = nullptr;
    size_t stride = 0;
    GetAccessorBaseAndStride(model, accessor, base, stride);

    out.reserve(out.size() + size_t(accessor.count));

    for (size_t i = 0; i < size_t(accessor.count); ++i)
        out.push_back(ReadWeightsVec4(accessor, base + i * stride));

    return true;
}

bool GltfImporter::ReadAccessorIndices(const tg::Model& model, const tg::Accessor& accessor, std::vector<uint32_t>& out,
                                       uint32_t offset)
{
    if (accessor.type != TINYGLTF_TYPE_SCALAR)
        return false;

    if (accessor.componentType != TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE &&
        accessor.componentType != TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT &&
        accessor.componentType != TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
        return false;

    const uint8_t* base = nullptr;
    size_t stride = 0;
    GetAccessorBaseAndStride(model, accessor, base, stride);

    out.reserve(out.size() + size_t(accessor.count));

    for (size_t i = 0; i < size_t(accessor.count); ++i)
        out.push_back(ReadComponentAsU32(base + i * stride, accessor.componentType) + offset);

    return true;
}

static bool registered = []()
{
    CookPipeline::RegisterImporter(REON::ASSET_MODEL, std::make_unique<GltfImporter>());
    return true;
}();
} // namespace REON::EDITOR