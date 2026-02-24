#include "reonpch.h"
#include "Material.h"
#include "REON/ResourceManagement/ResourceManager.h"
#include <REON/Platform/Vulkan/VulkanContext.h>
#include "REON/Application.h"

namespace REON {
    Material::Material() {
        const VulkanContext* context = static_cast<const VulkanContext*>(Application::Get().GetRenderContext());

        VkDeviceSize bufferSize = sizeof(FlatData);

        flatDataBuffers.resize(context->MAX_FRAMES_IN_FLIGHT);
        m_FlatDataBufferAllocations.resize(context->MAX_FRAMES_IN_FLIGHT);
        flatDataBuffersMapped.resize(context->MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < context->MAX_FRAMES_IN_FLIGHT; i++)
        {
            // TODO: use the VMA_ALLOCATION_CREATE_MAPPED_BIT along with VmaAllocationInfo object instead
            context->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                  VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, flatDataBuffers[i],
                                  m_FlatDataBufferAllocations[i]);

            vmaMapMemory(context->getAllocator(), m_FlatDataBufferAllocations[i], &flatDataBuffersMapped[i]);
        }
	}

	Material::~Material()
	{
		const VulkanContext* context = static_cast<const VulkanContext*>(Application::Get().GetRenderContext());

		for (size_t i = 0; i < context->MAX_FRAMES_IN_FLIGHT; i++) {
			vkDestroyBuffer(context->getDevice(), flatDataBuffers[i], nullptr);
			vmaUnmapMemory(context->getAllocator(), m_FlatDataBufferAllocations[i]);
			vmaFreeMemory(context->getAllocator(), m_FlatDataBufferAllocations[i]);
		}
	}

	
}