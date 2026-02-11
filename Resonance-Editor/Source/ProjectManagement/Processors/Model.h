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
#include "nlohmann/json.hpp"

struct aiNode;
struct aiMesh;

namespace REON {
    class Shader;
    struct LightData;
}


namespace REON::EDITOR {

    class Model {
    public:

        static void LoadModelToGameObject(const char filePath[], const std::shared_ptr<GameObject>& parentObject);

        static std::shared_ptr<GameObject> ConstructGameObjectFromModelFile(const std::filesystem::path modelPath, std::shared_ptr<Scene> scene);

    private:
        static std::shared_ptr<GameObject> ProcessModelNode(nlohmann::json json, nlohmann::json fullJson, std::shared_ptr<GameObject> nodeObject);

        void DecomposeTransform(const glm::mat4& transform, glm::vec3& translation, Quaternion& rotation, glm::vec3& scale);

    private:
        std::string m_Directory;
    };

}

