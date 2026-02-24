#include "reonpch.h"

#include "Model.h"

#define GLM_ENABLE_EXPERIMENTAL

#include "ProjectManagement/ProjectManager.h"
#include "REON/GameHierarchy/Components/Renderer.h"
#include "REON/GameHierarchy/Components/Transform.h"
#include "REON/Rendering/Shader.h"
#include "REON/Rendering/Structs/LightData.h"

#include <REON/ResourceManagement/ResourceManager.h>
#include <assimp/pbrmaterial.h>
#include <glm/gtx/matrix_decompose.hpp>

namespace REON::EDITOR
{

// Helper function to decompose glm::mat4 into translation, rotation, and scale
void Model::DecomposeTransform(const glm::mat4& transform, glm::vec3& translation, Quaternion& rotation,
                               glm::vec3& scale)
{
    glm::vec3 skew;
    glm::vec4 perspective;
    glm::decompose(transform, scale, rotation, translation, skew, perspective);
}

std::shared_ptr<GameObject> Model::ConstructGameObjectFromModelFile(const std::filesystem::path modelPath,
                                                                    std::shared_ptr<Scene> scene)
{
    std::ifstream file(modelPath.string() + ".meta");
    if (!file.is_open())
    {
        REON_CORE_ERROR("Failed to open companion file for reading: {}", modelPath.string());
        return nullptr;
    }

    nlohmann::json j;
    file >> j;
    file.close();

    if (j.contains("rootNodes"))
    {
        if (j["rootNodes"].size() > 1)
        {
            auto sceneObject = std::make_shared<GameObject>();

            scene->AddGameObject(sceneObject);

            if (j.contains("sceneName"))
            {
                sceneObject->SetName(j["sceneName"]);
            }
            else
            {
                sceneObject->SetName("Scene_" + std::chrono::duration_cast<std::chrono::milliseconds>(
                                                    std::chrono::system_clock::now().time_since_epoch())
                                                    .count());
            }

            for (auto node : j["rootNodes"])
            {
                auto nodeObject = std::make_shared<GameObject>();

                sceneObject->AddChild(nodeObject);

                ProcessModelNode(node, j, nodeObject); // ADD CHILD BEFORE SCENE IS SET
            }

            return sceneObject;
        }
        else
        {
            auto nodeObject = std::make_shared<GameObject>();

            scene->AddGameObject(nodeObject);

            return ProcessModelNode(j["rootNodes"][0], j, nodeObject);
        }
    }
    return nullptr;
}

std::shared_ptr<GameObject> Model::ProcessModelNode(nlohmann::json nodeJson, nlohmann::json fullJson,
                                                    std::shared_ptr<GameObject> nodeObject)
{
    static int objectCount = 1;

    std::vector<std::string> materialIDs;
    std::vector<ResourceHandle<Material>> materials;

    std::string defaultName = "Unnamed" + std::to_string(objectCount++);
    std::string name = nodeJson.value("name", defaultName);
    if (name.empty())
        name = defaultName;

    nodeObject->SetName(name);

    if (nodeJson.contains("transform"))
    {
        nodeObject->GetTransform()->SetFromMatrix(nodeJson["transform"]);
    }

    /*if (nodeJson.contains("materials")) {
        for (const auto& materialID : nodeJson["materials"]) {
            std::shared_ptr<Material> mat;

            if (!(mat = ResourceManager::GetInstance().GetResource<Material>(materialID))) {
                if (auto matInfo = AssetRegistry::Instance().GetAssetById(materialID)) {
                    mat = std::make_shared<Material>();
                    mat->Deserialize(ProjectManager::GetInstance().GetCurrentProjectPath() + "\\" +
    matInfo->path.string(), ProjectManager::GetInstance().GetCurrentProjectPath());
                    ResourceManager::GetInstance().AddResource(mat);
                }
                else {
                    REON_ERROR("CANT FIND MATERIAL: {}", materialID.get<std::string>());
                }
            }
            REON_CORE_ASSERT(mat);
            if(std::find(materialIDs.begin(), materialIDs.end(), materialID.get<std::string>()) == materialIDs.end()) {
                materialIDs.push_back(materialID);
                materials.push_back(mat);
            }
        }
    }*/

    if (nodeJson.contains("meshIDs"))
    {
        nlohmann::json meshJson;
        for (const auto& meshID : nodeJson["meshIDs"])
        {
            meshJson = "";
            for (const auto& mesh : fullJson["meshes"])
            {
                if (mesh.contains("GUID") && mesh["GUID"] == meshID)
                {
                    meshJson = mesh;
                }
            }
            if (meshJson.empty())
            {
                REON_ERROR("MESHDATA NOT FOUND IN JSON");
                continue;
            }

            std::shared_ptr<Mesh> mesh;

            // if (!(mesh = ResourceManager::GetInstance().GetResource<Mesh>(meshID))) {
            //	mesh = std::make_shared<Mesh>(meshID);

            //	mesh->DeSerialize(meshJson);

            //	ResourceManager::GetInstance().AddResource(mesh);
            //}

            std::shared_ptr<Renderer> renderer = {}; // std::make_shared<Renderer>(mesh, materials);
            nodeObject->AddComponent(renderer);
        }
    }

    if (nodeJson.contains("children"))
    {
        for (const auto& childNode : nodeJson["children"])
        {
            std::shared_ptr<GameObject> childObject = std::make_shared<GameObject>();
            nodeObject->AddChild(childObject);
            ProcessModelNode(childNode, fullJson, childObject);
        }
    }

    return nodeObject;
}
} // namespace REON::EDITOR