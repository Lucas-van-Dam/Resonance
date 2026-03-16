#pragma once

#include "glm/glm.hpp"
#include <vulkan/vulkan.h>
#include <array>

namespace REON{

    struct Vertex {
        glm::vec3 Position;
		glm::vec4 Color;
        glm::vec3 Normal;
        glm::vec2 TexCoords;
        glm::vec4 Tangent;
        glm::u16vec4 Joints_0;
        glm::u16vec4 Joints_1;
        glm::vec4 Weights_0;
        glm::vec4 Weights_1;

		static VkVertexInputBindingDescription getBindingDescription() {
			VkVertexInputBindingDescription bindingDescription{};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(Vertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescription;
		}

		static std::array<VkVertexInputAttributeDescription, 9> getAttributeDescriptions() {
			std::array<VkVertexInputAttributeDescription, 9> attributeDescriptions{};

			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[0].offset = offsetof(Vertex, Position);

			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(Vertex, Color);

			attributeDescriptions[2].binding = 0;
			attributeDescriptions[2].location = 2;
			attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[2].offset = offsetof(Vertex, Normal);

			attributeDescriptions[3].binding = 0;
			attributeDescriptions[3].location = 3;
			attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[3].offset = offsetof(Vertex, TexCoords);

			attributeDescriptions[4].binding = 0;
			attributeDescriptions[4].location = 4;
			attributeDescriptions[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
			attributeDescriptions[4].offset = offsetof(Vertex, Tangent);

			attributeDescriptions[5].binding = 0;
            attributeDescriptions[5].location = 5;
            attributeDescriptions[5].format = VK_FORMAT_R16G16B16A16_UINT;
            attributeDescriptions[5].offset = offsetof(Vertex, Joints_0);

			attributeDescriptions[6].binding = 0;
            attributeDescriptions[6].location = 6;
            attributeDescriptions[6].format = VK_FORMAT_R16G16B16A16_UINT;
            attributeDescriptions[6].offset = offsetof(Vertex, Joints_1);

			attributeDescriptions[7].binding = 0;
            attributeDescriptions[7].location = 7;
            attributeDescriptions[7].format = VK_FORMAT_R32G32B32A32_SFLOAT;
            attributeDescriptions[7].offset = offsetof(Vertex, Weights_0);

            attributeDescriptions[8].binding = 0;
            attributeDescriptions[8].location = 8;
            attributeDescriptions[8].format = VK_FORMAT_R32G32B32A32_SFLOAT;
            attributeDescriptions[8].offset = offsetof(Vertex, Weights_1);

			return attributeDescriptions;
		}
    };

}