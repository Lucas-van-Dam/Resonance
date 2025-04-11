#include "reonpch.h"
#include "RenderCommand.h"

#include "glad/glad.h"
#include "Mesh.h"

namespace REON {
    void RenderCommand::Init() {
        glEnable(GL_DEPTH_TEST);
        // etc.
    }

    void RenderCommand::SetClearColor(const glm::vec4& color) {
        glClearColor(color.r, color.g, color.b, color.a);
    }

    void RenderCommand::Clear() {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void RenderCommand::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
        glViewport(x, y, width, height);
    }

    void RenderCommand::DrawIndexed(uint32_t vao, uint32_t indexCount) {
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);
    }
}