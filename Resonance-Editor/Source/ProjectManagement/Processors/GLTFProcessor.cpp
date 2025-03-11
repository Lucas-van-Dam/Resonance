#include "reonpch.h"
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "GLTFProcessor.h"
#include <ProjectManagement/ProjectManager.h>
#include <REON/ResourceManagement/ResourceInfo.h>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

using json = nlohmann::json;

namespace REON::EDITOR {

	void GLTFProcessor::Process(AssetInfo& assetInfo)
	{
		tg::Model model;
		tg::TinyGLTF loader;
		std::string err;
		std::string warn;

		auto extension = assetInfo.path.extension().string();
		std::string fullPath = ProjectManager::GetInstance().GetCurrentProjectPath() + "\\" + assetInfo.path.string();

		if (extension == ".gltf") {
			if (!loader.LoadASCIIFromFile(&model, &err, &warn, fullPath)) {
				REON_ERROR("Failed to parse GLTF: {}", err);
			}
		}
		else if (extension == ".glb") {
			if (!loader.LoadBinaryFromFile(&model, &err, &warn, fullPath)) {
				REON_ERROR("Failed to parse GLTF: {}", err);
			}
		}
		else
			REON_ERROR("File was not recognised as a gltf file.");

		if (!err.empty()) {
			REON_ERROR("Error while opening gltf file: {}", err);
		}

		if (!warn.empty()) {
			REON_WARN("Warning while opening gltf file: {}", warn);
		}

		uid = assetInfo.id;
		basePath = assetInfo.path;

		MetaFileData metaData;
		metaData.modelUID = assetInfo.id;

		//std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>();

		//ResourceManager::GetInstance().AddResource(mesh);

		materialIDs.clear();

		for (auto& srcMat : model.materials) {
			auto litShader = ResourceManager::GetInstance().LoadResource<Shader>("DefaultLit", std::make_tuple("PBR.vert", "PBR.frag", std::optional<std::string>{}));

			std::shared_ptr<Material> mat = std::make_shared<Material>();
			mat->shader = litShader;

			mat->SetName(srcMat.name);

			materialIDs.push_back(mat->GetID());

			if (srcMat.alphaMode != "OPAQUE")
				REON_WARN("Transparent materials are not supported yet, material: {}", srcMat.name);

			auto pbrData = srcMat.pbrMetallicRoughness;

			mat->albedoColor = glm::vec4(
				static_cast<float>(pbrData.baseColorFactor[0]),
				static_cast<float>(pbrData.baseColorFactor[1]),
				static_cast<float>(pbrData.baseColorFactor[2]),
				static_cast<float>(pbrData.baseColorFactor[3]));
			
			if (pbrData.baseColorTexture.index >= 0) {
				REON_ERROR("HAVENT IMPLEMENTED COLOR TEXTURES YET, WILL IN A BIT");
			}

			if (pbrData.metallicRoughnessTexture.index >= 0) {
				REON_ERROR("HAVENT IMPLEMENTED METALLICROUGHNESS TEXTURES YET, WILL IN A BIT");
			}
			else {
				mat->roughness = pbrData.roughnessFactor;
				mat->metallic = pbrData.metallicFactor;
			}

			if (srcMat.normalTexture.index >= 0) {
				REON_ERROR("HAVENT IMPLEMENTED NORMAL TEXTURES YET, WILL IN A BIT");
			}

			AssetRegistry::Instance().RegisterAsset({mat->GetID(), "Material", mat->Serialize(ProjectManager::GetInstance().GetCurrentProjectPath() + "\\" + (basePath.parent_path().string())).string()});

			ResourceManager::GetInstance().AddResource(mat);
		}

		const int sceneToLoad = model.defaultScene < 0 ? 0 : model.defaultScene;

		for (auto& nodeId : model.scenes[sceneToLoad].nodes) {
			if (nodeId < 0)
				return;

			metaData.rootNode = HandleGLTFNode(model, model.nodes[nodeId], glm::mat4(1.0f), metaData);
		}

		assetInfo.extraInformation["Importer"] = "GLTFImporter";

		std::string metaDataPath = ProjectManager::GetInstance().GetCurrentProjectPath() + "\\"
			+ assetInfo.path.parent_path().string() + "\\" + assetInfo.path.stem().string() + ".model";

		SerializeCompanionFile(metaData, metaDataPath);
	}

	SceneNodeData GLTFProcessor::HandleGLTFNode(const tg::Model& model, const tg::Node& node, const glm::mat4& parentTransform, MetaFileData& modelFileData)
	{
		SceneNodeData data;
		data.name = node.name;

		glm::mat4 transform = parentTransform * GetTransformFromGLTFNode(node);

		data.transform = transform;

		int meshId = node.mesh;

		if (meshId >= 0 && meshId < model.meshes.size()) {
			auto meshID = HandleGLTFMesh(model, model.meshes[meshId], transform, data);
			data.meshIDs.push_back(meshID);
			modelFileData.meshes.push_back(ResourceManager::GetInstance().GetResource<Mesh>(meshID));
		}

		for (auto& nodeId : node.children) {
			if (nodeId < 0 || nodeId > model.nodes.size())
				continue;
			
			data.children.push_back(HandleGLTFNode(model, model.nodes[nodeId], transform, modelFileData));
		}

		return data;
	}

	std::string GLTFProcessor::HandleGLTFMesh(const tg::Model& model, const tg::Mesh& mesh, const glm::mat4& transform, SceneNodeData& sceneNode)
	{
		std::shared_ptr<Mesh> meshData = std::make_shared<Mesh>();
		meshData->SetName(mesh.name);

		std::vector<std::string> matIDsPerMesh;

		int primitiveIndex = 0;
		for (auto primitive : mesh.primitives) {
			SubMesh subMesh;
			subMesh.indexOffset = meshData->indices.size();

			auto matID = materialIDs[primitive.material];
			auto it = std::find(matIDsPerMesh.begin(), matIDsPerMesh.end(), matID);
			if (it != matIDsPerMesh.end()) {
				subMesh.materialIndex = std::distance(matIDsPerMesh.begin(), it);
			}
			else {
				subMesh.materialIndex = matIDsPerMesh.size();
				matIDsPerMesh.push_back(matID);
			}
			sceneNode.materials.push_back(matID);

			const int vertexOffset = meshData->vertices.size();

			for (auto& attribute : primitive.attributes)
			{
				const tinygltf::Accessor& accessor = model.accessors.at(attribute.second);
				if (attribute.first.compare("POSITION") == 0)
				{
					HandleGLTFBuffer(model, accessor, meshData->vertices, transform);
				}
				else if (attribute.first.compare("NORMAL") == 0)
				{
					HandleGLTFBuffer(model, accessor, meshData->normals, transform, true);
				}
				else if (attribute.first.compare("TEXCOORD_0") == 0)
				{
					HandleGLTFBuffer(model, accessor, meshData->uvs);
				}
				else if (attribute.first.compare("COLOR_0") == 0)
				{
					REON_INFO("Color found");
					//auto result = detail::handleGltfVertexColor(model, accessor, accessor.type, accessor.componentType, meshData.colors);
				}
				else
				{
					REON_WARN("More data in GLTF file, accessor name: {}", attribute.first);
				}
			}

			const int smallestBufferSize = std::min(meshData->normals.size(), std::min(meshData->uvs.size(), meshData->colors.size()));
			const int vertexCount = meshData->vertices.size();
			meshData->normals.reserve(vertexCount);
			meshData->uvs.reserve(vertexCount);
			meshData->colors.reserve(vertexCount);

			for (int i = smallestBufferSize; i < vertexCount; ++i)
			{
				if (meshData->normals.size() == i)
					meshData->normals.push_back(glm::vec3(0,1,0));

				if (meshData->uvs.size() == i)
					meshData->uvs.push_back(glm::vec2(0, 0));

				if (meshData->colors.size() == i)
					meshData->colors.push_back(glm::vec4(1,1,1,1));
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
				for (uint i = static_cast<uint>(vertexOffset); i < meshData->vertices.size(); i++)
					meshData->indices.push_back(i);
			}
			subMesh.indexCount = meshData->indices.size() - subMesh.indexOffset;

			primitiveIndex++;

			meshData->subMeshes.push_back(subMesh);
		}

		ResourceManager::GetInstance().AddResource(meshData);
		AssetRegistry::Instance().RegisterAsset({ meshData->GetID(), "Mesh", basePath.string() + ".meta"});

		return meshData->GetID();
	}

	glm::mat4 GLTFProcessor::GetTransformFromGLTFNode(const tg::Node& node)
	{
		if (node.matrix.size() == 16) {
			return glm::mat4(
				static_cast<float>(node.matrix[0]), static_cast<float>(node.matrix[1]), static_cast<float>(node.matrix[2]), static_cast<float>(node.matrix[3]),
				static_cast<float>(node.matrix[4]), static_cast<float>(node.matrix[5]), static_cast<float>(node.matrix[6]), static_cast<float>(node.matrix[7]),
				static_cast<float>(node.matrix[8]), static_cast<float>(node.matrix[9]), static_cast<float>(node.matrix[10]), static_cast<float>(node.matrix[11]),
				static_cast<float>(node.matrix[12]), static_cast<float>(node.matrix[13]), static_cast<float>(node.matrix[14]), static_cast<float>(node.matrix[15]));
		}
		else {
			glm::vec3 trans{0,0,0};
			Quaternion rot{1,0,0,0};
			glm::vec3 scale{1,1,1};

			if (node.scale.size() == 3)
				scale = glm::vec3(static_cast<float>(node.scale[0]), static_cast<float>(node.scale[1]), static_cast<float>(node.scale[2]));

			if (node.rotation.size() == 4)
				rot = Quaternion(static_cast<float>(node.rotation[3]), static_cast<float>(node.rotation[0]), static_cast<float>(node.rotation[1]), static_cast<float>(node.rotation[2]));

			if (node.translation.size() == 3)
				trans = glm::vec3(static_cast<float>(node.translation[0]), static_cast<float>(node.translation[1]), static_cast<float>(node.translation[2]));

			glm::mat4 translation = glm::translate(glm::mat4(1.0f), trans);
			glm::mat4 rotation = glm::toMat4(rot);
			glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), scale);

			return(translation * rotation * scaleMat);
		}
	}

	void GLTFProcessor::HandleGLTFBuffer(const tg::Model& model, const tg::Accessor& accessor, std::vector<glm::vec3>& data, const glm::mat4& transform, bool normal)
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
				data.push_back(glm::vec3((transform * glm::vec4(*x, *y, *z, 0.f))));
			}
		}
		else
		{
			for (size_t i = bufferStart; i < bufferEnd; i += stride)
			{
				const float* x = reinterpret_cast<const float*>(&buffer.data[i]);
				const float* y = reinterpret_cast<const float*>(&buffer.data[i + sizeof(float)]);
				const float* z = reinterpret_cast<const float*>(&buffer.data[i + 2 * sizeof(float)]);
				data.push_back(glm::vec3((transform * glm::vec4(*x, *y, *z, 1.f))));
			}
		}

		if (accessor.sparse.isSparse)
			REON_WARN("Sparse accessors are not supported yet (while processing buffer)");
	}

	template<typename T>
	void GLTFProcessor::HandleGLTFBuffer(const tinygltf::Model& model, const tinygltf::Accessor& accessor, std::vector<T>& data)
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

	void GLTFProcessor::HandleGLTFIndices(const tg::Model& model, const tg::Accessor& accessor, int offset, std::vector<uint>& data) 
	{
		const tinygltf::BufferView& bufferView = model.bufferViews.at(accessor.bufferView);
		const tinygltf::Buffer& buffer = model.buffers.at(bufferView.buffer);
		const int bufferStart = bufferView.byteOffset + accessor.byteOffset;
		const int stride = accessor.ByteStride(bufferView);
		const int bufferEnd = bufferStart + accessor.count * stride;

		const int dataStart = data.size();
		data.reserve(dataStart + accessor.count);

		switch (accessor.componentType) {
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
		j["rootNode"] = SerializeSceneNode(data.rootNode);
		
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



	//void GLTFProcessor::ProcessNode(aiNode* node, const aiScene* scene, std::string path) {
	//	if (node->mNumMeshes > 0) {
	//		for (int i = 0; i < node->mNumMeshes; i++) {
	//			ProcessMesh(scene->mMeshes[node->mMeshes[i]], scene, path);
	//		}
	//	}

	//	for (int i = 0; i < node->mNumChildren; i++) {
	//		ProcessNode(node->mChildren[i], scene, path);
	//	}
	//}

	//void GLTFProcessor::ProcessMesh(aiMesh* mesh, const aiScene* scene, std::string path) {
	//	std::vector<Vertex> vertices;
	//	std::vector<unsigned int> indices;
	//	std::string meshIdentifier = path.substr(0, path.find_last_of('/')) + "/" + mesh->mName.C_Str();



	//	std::shared_ptr<Mesh> meshObj = ResourceManager::GetInstance().LoadResource<Mesh>(meshIdentifier, std::make_tuple(vertices, indices));

	//	meshObj->Serialize(ProjectManager::GetInstance().GetCurrentProjectPath() + "\\EngineCache\\Meshes");

	//	ResourceInfo info;
	//	info.path = path;
	//	info.UID = uid;
	//	info.localIdentifier = localIdentifier++;

	//	json infoData;
	//	infoData["UID"] = info.UID;
	//	infoData["Path"] = info.path;
	//	infoData["localIdentifier"] = info.localIdentifier;

	//	std::ofstream infoFile((ProjectManager::GetInstance().GetCurrentProjectPath() + "\\EngineCache\\Meshes\\" + meshObj->GetID() + ".mesh.info"));
	//	if (infoFile.is_open()) {
	//		infoFile << infoData.dump(4);
	//		infoFile.close();
	//	}
	//	AssetRegistry::Instance().RegisterAsset({ meshObj->GetID(), "Mesh", "EngineCache/Meshes/" + meshObj->GetID() + ".mesh" });
	//}
}
