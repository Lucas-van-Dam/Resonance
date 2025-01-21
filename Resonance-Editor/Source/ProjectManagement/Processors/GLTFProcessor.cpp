#include "reonpch.h"
#include "GLTFProcessor.h"
#include <ProjectManagement/ProjectManager.h>



namespace REON::EDITOR {

	void GLTFProcessor::Process(const AssetInfo& assetInfo)
	{
		Assimp::DefaultLogger::create("", Assimp::Logger::VERBOSE);
		AssetProcessorLogStream* stderrStream = new AssetProcessorLogStream();
		Assimp::DefaultLogger::get()->attachStream(stderrStream, Assimp::Logger::VERBOSE);
		std::string path = assetInfo.path.string();

		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals |
			aiProcess_CalcTangentSpace | aiProcess_PreTransformVertices);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
			REON_CORE_WARN("Assimp scene not valid: {}", importer.GetErrorString());
			return;
		}

		ProcessNode(scene->mRootNode, scene, path);

		Assimp::DefaultLogger::kill();
	}

	void GLTFProcessor::ProcessNode(aiNode* node, const aiScene* scene, std::string path) {
		if (node->mNumMeshes > 0) {
			for (int i = 0; i < node->mNumMeshes; i++) {
				ProcessMesh(scene->mMeshes[node->mMeshes[i]], scene, path);
			}
		}

		for (int i = 0; i < node->mNumChildren; i++) {
			ProcessNode(node->mChildren[i], scene, path);
		}
	}

	void GLTFProcessor::ProcessMesh(aiMesh* mesh, const aiScene* scene, std::string path) {
		std::vector<Vertex> vertices;
		std::vector<unsigned int> indices;
		std::string meshIdentifier = path + "/" + mesh->mName.C_Str();

		// walk through each of the mesh's vertices
		for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
			Vertex vertex;
			glm::vec3 vector; // we declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.
			// positions
			vector.x = mesh->mVertices[i].x;
			vector.y = mesh->mVertices[i].y;
			vector.z = mesh->mVertices[i].z;
			vertex.Position = vector;
			// normals
			if (mesh->HasNormals()) {
				vector.x = mesh->mNormals[i].x;
				vector.y = mesh->mNormals[i].y;
				vector.z = mesh->mNormals[i].z;
				vertex.Normal = vector;
			}
			// texture coordinates
			if (mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
			{
				glm::vec2 vec;
				// a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't
				// use models where a vertex can have multiple texture coordinates so we always take the first set (0).
				vec.x = mesh->mTextureCoords[0][i].x;
				vec.y = mesh->mTextureCoords[0][i].y;
				vertex.TexCoords = vec;
				// tangent
				vector.x = mesh->mTangents[i].x;
				vector.y = mesh->mTangents[i].y;
				vector.z = mesh->mTangents[i].z;
				vertex.Tangent = vector;
				// bitangent
				vector.x = mesh->mBitangents[i].x;
				vector.y = mesh->mBitangents[i].y;
				vector.z = mesh->mBitangents[i].z;
				vertex.Bitangent = vector;
			}
			else
				vertex.TexCoords = glm::vec2(0.0f, 0.0f);

			vertices.push_back(vertex);
		}
		// now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
		for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
			aiFace face = mesh->mFaces[i];
			// retrieve all indices of the face and store them in the indices vector
			for (unsigned int j = 0; j < face.mNumIndices; j++)
				indices.push_back(face.mIndices[j]);
		}

		std::shared_ptr<Mesh> meshObj = ResourceManager::GetInstance().LoadResource<Mesh>(meshIdentifier, std::make_tuple(vertices, indices));

		meshObj->Serialize(ProjectManager::GetInstance().GetCurrentProjectPath() + "EngineCache/Meshes/" + meshObj->GetID() + ".mesh");

		AssetRegistry::Instance().RegisterAsset({ meshObj->GetID(), "Mesh", "EngineCache/Meshes/" + meshObj->GetID() + ".mesh" });
	}

}
