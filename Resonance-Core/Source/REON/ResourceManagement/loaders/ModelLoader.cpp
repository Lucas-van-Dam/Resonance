#include "reonpch.h"
#include "ModelLoader.h"

#include "ModelBinContainerReader.h"
#include "REON/GameHierarchy/Components/Transform.h"
#include "REON/GameHierarchy/GameObject.h"
#include "SceneLoader.h"

#include <REON/Application.h>
#include <REON/EngineServices.h>
#include <filesystem>

namespace REON
{
static std::string MakeNodeName(size_t i)
{
    return "Node_" + std::to_string(i);
}

static void SetTransformFromTRS(std::shared_ptr<GameObject>& go, const SceneNode& n)
{
    // Replace with your Transform API
    go->GetTransform()->localPosition = {n.t[0], n.t[1], n.t[2]};
    go->GetTransform()->localRotation = {n.r[0], n.r[1], n.r[2], n.r[3]}; // quat
    go->GetTransform()->localScale = {n.s[0], n.s[1], n.s[2]};
}


// Converts 16-byte array in SceneNode to your AssetId type.
static AssetId AssetIdFromBytes16(const uint8_t b[16])
{
    AssetId id{};
    std::memcpy(id.bytes.data(), b, 16);
    return id;
}

static bool SceneNodeHasMesh(const SceneNode& n)
{
    return !IsNull(AssetIdFromBytes16(n.meshId));
}


static std::shared_ptr<GameObject> BuildNodeRecursive(uint32_t nodeIndex, const std::vector<SceneNode>& nodes,
                                                      std::shared_ptr<Scene>& scene, const std::shared_ptr<GameObject>& parent)
{
    const SceneNode& n = nodes[nodeIndex];

    auto obj = std::make_shared<GameObject>();
    obj->SetName(MakeNodeName(nodeIndex));

    // Attach to scene graph
    if (parent)
        parent->AddChild(obj);
    else
        scene->AddGameObject(obj);

    SetTransformFromTRS(obj, n);

    if (SceneNodeHasMesh(n))
    {
        AssetId meshId = AssetIdFromBytes16(n.meshId);
        std::vector<AssetId> materialId(10, NullAssetId);

        std::memcpy(materialId.data(), n.materialId, sizeof(n.materialId));

        auto mesh = Application::Get().GetEngineServices().resources.GetOrLoad<Mesh>(meshId);
        std::vector<ResourceHandle<Material>> materialHandles;
        for (const auto& matId : materialId)
        {
            if (!IsNull(matId))
                materialHandles.push_back(Application::Get().GetEngineServices().resources.GetOrLoad<Material>(matId));
        }
        auto renderer = std::make_shared<Renderer>(mesh, materialHandles);
        obj->AddComponent(renderer);
    }

    // Recurse through children using firstChild/nextSibling chain
    uint32_t child = n.firstChild;
    while (child != UINT32_MAX)
    {
        BuildNodeRecursive(child, nodes, scene, obj);
        child = nodes[child].nextSibling;
    }

    return obj;
}

static std::vector<uint32_t> CollectRoots(const std::vector<SceneNode>& nodes)
{
    std::vector<uint32_t> roots;
    for (uint32_t i = 0; i < (uint32_t)nodes.size(); ++i)
        if (nodes[i].parent == UINT32_MAX)
            roots.push_back(i);
    return roots;
}

bool ModelLoader::LoadModelFromFile(AssetId id, std::shared_ptr<Scene> scene)
{
    ArtifactRef modelRef{};
    auto& services = Application::Get().GetEngineServices();
    if (!services.resolver->Resolve({ASSET_MODEL, id}, modelRef))
        return false;

    ModelBinContainerReader container;
    if (!container.Open(modelRef, *services.blobReader))
        return false;

    std::vector<SceneNode> nodes;
    if (!SceneLoader::LoadSceneNodes(container, *services.blobReader, nodes))
        return false;

    auto roots = CollectRoots(nodes);
    if (roots.empty())
        return false;

    if (roots.size() > 1)
    {
        auto rootObj = std::make_shared<GameObject>();
        scene->AddGameObject(rootObj);

        rootObj->SetName("Model_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())); // or timestamp like before

        for (uint32_t r : roots)
            BuildNodeRecursive(r, nodes, scene, rootObj);
    }
    else
    {
        // Single root: same behavior as your JSON path
        BuildNodeRecursive(roots[0], nodes, scene, nullptr);
    }

    return true;
}
} // namespace REON