#include "reonpch.h"
#include "Renderer.h"

#include "RenderCommand.h"

namespace REON {
    void Renderer::Init() {
        RenderCommand::Init();
        // ... set up default state
    }

    void Renderer::BeginFrame() {
        RenderCommand::Clear();
    }

    void Renderer::EndFrame() {
        // Swap buffers if needed (your window system may handle this)
    }

    void Renderer::SubmitMesh(const std::shared_ptr<Mesh>& mesh, const std::shared_ptr<Material>& material, const glm::mat4& transform) {
        //material->Bind(); // shader + uniforms
        //mesh->GetVertexArray()->Bind();

        //// Upload model matrix
        //material->GetShader()->SetMat4("u_Model", transform); // adapt this to your shader API

        //RenderCommand::DrawIndexed(mesh->GetVertexArray());
    }
}