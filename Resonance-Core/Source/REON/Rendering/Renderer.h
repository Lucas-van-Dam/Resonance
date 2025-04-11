#pragma once

#include <memory>
#include "REON/Rendering/Mesh.h"
#include "REON/Rendering/Material.h"

namespace REON {
    class Renderer {
    public:
        static void Init();
        static void Shutdown();

        static void BeginFrame();
        static void EndFrame();

        static void SubmitMesh(
            const std::shared_ptr<Mesh>& mesh,
            const std::shared_ptr<Material>& material,
            const glm::mat4& transform
        );
    };
}