#pragma once

#include "glm/glm.hpp"

namespace REON {
    class RenderCommand {
    public:
        static void Init();
        static void SetClearColor(const glm::vec4& color);
        static void Clear();
        static void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height);
        static void DrawIndexed(uint32_t vao, uint32_t indexCount = 0);
    };

}