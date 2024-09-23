#include "tekpch.h"
#include "Model.h"

#define GLM_ENABLE_EXPERIMENTAL

#include "Tek/Rendering/Shader.h"
#include "Tek/Rendering/Structs/LightData.h"
#include "Tek/Rendering/Structs/Texture.h"
#include "Tek/Rendering/Mesh.h"
#include "Tek/GameHierarchy/Components/Renderer.h"
#include "Tek/GameHierarchy/Components/Transform.h"
#include <assimp/pbrmaterial.h>
#include <glm/gtx/matrix_decompose.hpp>


namespace TEK {

    // Helper function to convert aiMatrix4x4 to glm::mat4
    glm::mat4 Model::ConvertToGLM(const aiMatrix4x4& aiMat) {
        glm::mat4 mat;
        mat[0][0] = aiMat.a1; mat[1][0] = aiMat.a2; mat[2][0] = aiMat.a3; mat[3][0] = aiMat.a4;
        mat[0][1] = aiMat.b1; mat[1][1] = aiMat.b2; mat[2][1] = aiMat.b3; mat[3][1] = aiMat.b4;
        mat[0][2] = aiMat.c1; mat[1][2] = aiMat.c2; mat[2][2] = aiMat.c3; mat[3][2] = aiMat.c4;
        mat[0][3] = aiMat.d1; mat[1][3] = aiMat.d2; mat[2][3] = aiMat.d3; mat[3][3] = aiMat.d4;
        return mat;
    }

    // Helper function to decompose glm::mat4 into translation, rotation, and scale
    void Model::DecomposeTransform(const glm::mat4& transform, glm::vec3& translation, Quaternion rotation, glm::vec3& scale) {
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(transform, scale, rotation, translation, skew, perspective);
    }

    // loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
    void Model::loadModel(std::string const& path, const std::shared_ptr<GameObject>& parent) {
        // read file via ASSIMP
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals |
            aiProcess_CalcTangentSpace | aiProcess_PreTransformVertices);
        // check for errors
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
        {
            std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
            return;
        }

        // retrieve the directory path of the filepath
        directory = path.substr(0, path.find_last_of('/'));

        // process ASSIMP's root node recursively
        processNode(scene->mRootNode, scene, parent);
    }

    // processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
    void Model::processNode(aiNode* node, const aiScene* scene, const std::shared_ptr<GameObject>& parent) {
        auto gameObject = std::make_shared<GameObject>();
        gameObject->SetName(node->mName.C_Str());
        parent->AddChild(gameObject);

        // Extract and apply the node's transformation
        glm::mat4 transform = ConvertToGLM(node->mTransformation);
        glm::vec3 translation, scale;
        Quaternion rotation;
        DecomposeTransform(transform, translation, rotation, scale);

        gameObject->GetTransform()->localPosition = translation;
        gameObject->GetTransform()->localRotation = rotation;
        gameObject->GetTransform()->localScale = scale;

        // process each mesh located at the current node
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            // the node object only contains indices to index the actual objects in the scene.
            // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            auto meshObject = processMesh(mesh, scene);
            gameObject->AddChild(meshObject);
        }
        // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            processNode(node->mChildren[i], scene, gameObject);
        }

    }

    std::shared_ptr<GameObject> Model::processMesh(aiMesh* mesh, const aiScene* scene) {
        auto gameObject = std::make_shared<GameObject>();
        gameObject->SetName(mesh->mName.C_Str());
        // data to fill
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        std::vector<Texture> textures;
        std::shared_ptr<Material> material = std::make_shared<Material>(std::make_shared<Shader>("PBR.vert", "PBR.frag"));

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
        // process materials
        aiMaterial* aiMaterial = scene->mMaterials[mesh->mMaterialIndex];
        // we assume a convention for sampler names in the shaders. Each diffuse texture should be named
        // as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER.
        // Same applies to other texture as the following list summarizes:
        // diffuse: texture_diffuseN
        // specular: texture_specularN
        // normal: texture_normalN

        auto color = aiColor4D(1, 1, 1, 1);
        aiGetMaterialColor(aiMaterial, AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_BASE_COLOR_FACTOR, &color);
        float metallic = -1;
        aiGetMaterialFloat(aiMaterial, AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLIC_FACTOR, &metallic);
        float roughness = -1;
        aiGetMaterialFloat(aiMaterial, AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_ROUGHNESS_FACTOR, &roughness);

        //    // 1. diffuse maps
        //    vector<Texture> diffuseMaps = loadMaterialTextures(aiMaterial, aiTextureType_DIFFUSE, "texture_diffuse");
        //    textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
        //    // 2. normal maps
        //    std::vector<Texture> normalMaps = loadMaterialTextures(aiMaterial, aiTextureType_NORMALS, "texture_normal");
        //    textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
        //    // 3. height maps
        //    std::vector<Texture> heightMaps = loadMaterialTextures(aiMaterial, aiTextureType_AMBIENT, "texture_height");
        //    textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());
        //    // 4. shininess maps
        //    vector<Texture> shininessMaps = loadMaterialTextures(aiMaterial, aiTextureType_SHININESS, "texture_shininess");
        //    textures.insert(textures.end(), shininessMaps.begin(), shininessMaps.end());
        //    // 5. metalness maps
        //    vector<Texture> metallicMaps = loadMaterialTextures(aiMaterial, aiTextureType_METALNESS, "texture_metallic");
        //    textures.insert(textures.end(), metallicMaps.begin(), metallicMaps.end());

        //TODO: ADD METALLIC ROUGHNESS TOGETHER

        material->AlbedoTexture = LoadMaterialTextures(aiMaterial, aiTextureType_DIFFUSE, "texture_diffuse", directory);
        material->NormalTexture = LoadMaterialTextures(aiMaterial, aiTextureType_NORMALS, "texture_normal", directory);
        material->RoughnessTexture = LoadMaterialTextures(aiMaterial, aiTextureType_SHININESS, "texture_shininess", directory);
        //material->MetallicTexture = LoadMaterialTextures(aiMaterial, aiTextureType_METALNESS, "texture_metallic", directory);

        material->Metallic = metallic;
        material->Roughness = roughness;
        material->AlbedoColor = glm::vec4(color.r, color.g, color.b, color.a);

        std::shared_ptr<Mesh> meshObj = std::make_shared<Mesh>(vertices, indices);

        std::shared_ptr<Renderer> renderer = std::make_shared<Renderer>(meshObj, material);
        gameObject->AddComponent(renderer);

        // return a mesh object created from the extracted mesh data
        return gameObject;
    }

    void Model::LoadModelToGameObject(const char filePath[], const std::shared_ptr<GameObject>& parentObject) {
        Model model(filePath, parentObject);

    }

}