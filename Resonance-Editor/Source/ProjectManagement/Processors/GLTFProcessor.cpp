#include "reonpch.h"
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "GLTFProcessor.h"
#include <ProjectManagement/ProjectManager.h>
#include <REON/ResourceManagement/ResourceInfo.h>
#include <glm/gtx/quaternion.hpp>

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
		localIdentifier = 0;

		//std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>();

		//ResourceManager::GetInstance().AddResource(mesh);

		const int sceneToLoad = model.defaultScene < 0 ? 0 : model.defaultScene;

		for (auto& nodeId : model.scenes[sceneToLoad].nodes) {
			if (nodeId < 0)
				return;

			HandleGLTFNode(model, model.nodes[nodeId], glm::mat4(1.0f));
		}

		assetInfo.extraInformation["Importer"] = "GLTFImporter";
	}

	void GLTFProcessor::HandleGLTFNode(const tg::Model& model, const tg::Node& node, const glm::mat4& parentTransform)
	{
		glm::mat4 transform = parentTransform * GetTransformFromGLTFNode(node);

		int meshId = node.mesh;

		if (meshId >= 0 && meshId < model.meshes.size()) {
			HandleGLTFMesh(model, model.meshes[meshId], transform);
		}

		for (auto& nodeId : node.children) {
			if (nodeId < 0 || nodeId > model.nodes.size())
				continue;

			HandleGLTFNode(model, model.nodes[nodeId], transform);
		}
	}

	void GLTFProcessor::HandleGLTFMesh(const tg::Model& model, const tg::Mesh& mesh, const glm::mat4& transform)
	{
		Mesh meshData;
		meshData.SetName(mesh.name);

		int primitiveIndex = 0;
		for (auto primitive : mesh.primitives) {
			SubMesh subMesh;
			subMesh.indexOffset = meshData.indices.size();
			subMesh.materialIndex = primitive.material;

			const int vertexOffset = meshData.vertices.size();

			for (auto& attribute : primitive.attributes)
			{
				const tinygltf::Accessor& accessor = model.accessors.at(attribute.second);
				if (attribute.first.compare("POSITION") == 0)
				{
					HandleGLTFBuffer(model, accessor, meshData.vertices, transform);
				}
				else if (attribute.first.compare("NORMAL") == 0)
				{
					HandleGLTFBuffer(model, accessor, meshData.normals, transform, true);
				}
				else if (attribute.first.compare("TEXCOORD_0") == 0)
				{
					HandleGLTFBuffer(model, accessor, meshData.uvs);
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

			const int smallestBufferSize = std::min(meshData.normals.size(), std::min(meshData.uvs.size(), meshData.colors.size()));
			const int vertexCount = meshData.vertices.size();
			meshData.normals.reserve(vertexCount);
			meshData.uvs.reserve(vertexCount);
			meshData.colors.reserve(vertexCount);

			for (int i = smallestBufferSize; i < vertexCount; ++i)
			{
				if (meshData.normals.size() == i)
					meshData.normals.push_back(glm::vec3(0,1,0));

				if (meshData.uvs.size() == i)
					meshData.uvs.push_back(glm::vec2(0, 0));

				if (meshData.colors.size() == i)
					meshData.colors.push_back(glm::vec4(1,1,1,1));
			}

			const int index = primitive.indices;

			if (index > model.accessors.size())
			{
				REON_WARN("Invalid index accessor index in primitive: {}", std::to_string(primitiveIndex));
				return;
			}

			if (primitive.indices >= 0)
			{
				const tg::Accessor& indexAccessor = model.accessors.at(index);
				HandleGLTFIndices(model, indexAccessor, vertexOffset, meshData.indices);
			}
			else
			{
				for (uint i = static_cast<uint>(vertexOffset); i < meshData.vertices.size(); i++)
					meshData.indices.push_back(i);
			}
			primitiveIndex++;

			meshData.subMeshes.push_back(subMesh);
		}

		AssetRegistry::Instance().RegisterAsset({ meshData.GetID(), "Mesh", "EngineCache/Meshes/" + meshData.GetID() + ".mesh" });
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
