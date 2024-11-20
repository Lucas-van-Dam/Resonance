#pragma once

#include "assimp/DefaultLogger.hpp"
#include "assimp/Logger.hpp"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include "REON/GameHierarchy/GameObject.h"
#include <glm/glm.hpp>
#include "REON/Math/Quaternion.h"
#include "REON/Rendering/Structs/Texture.h"
#include "REON/Rendering/Mesh.h"

struct aiNode;
struct aiMesh;


namespace REON {
    class Shader;
    struct LightData;

    class Model {
    public:
        explicit Model(const char* path, const std::shared_ptr<GameObject>& parent) {
            Assimp::DefaultLogger::create("", Assimp::Logger::VERBOSE);
            Assimp::LogStream* stderrStream = Assimp::LogStream::createDefaultStream(aiDefaultLogStream_STDERR);
            Assimp::DefaultLogger::get()->attachStream(stderrStream, Assimp::Logger::VERBOSE | Assimp::Logger::Debugging |
                Assimp::Logger::VERBOSE);
            loadModel(path, parent);
            Assimp::DefaultLogger::kill();
        }

        static void LoadModelToGameObject(const char filePath[], const std::shared_ptr<GameObject>& parentObject);

    private:
        glm::mat4 ConvertToGLM(const aiMatrix4x4& aiMat);
        void DecomposeTransform(const glm::mat4& transform, glm::vec3& translation, Quaternion rotation, glm::vec3& scale);
        void loadModel(const std::string& path, const std::shared_ptr<GameObject>& parent);

        void processNode(aiNode* node, const aiScene* scene, const std::shared_ptr<GameObject>& parent);

        void processMesh(aiMesh* mesh, const aiScene* scene, std::shared_ptr<GameObject> parent);

        std::shared_ptr<Texture> LoadTexture(aiMaterial* mat, aiTextureType type, const std::string& directory);

    private:
        std::string m_Directory;
    };

}

