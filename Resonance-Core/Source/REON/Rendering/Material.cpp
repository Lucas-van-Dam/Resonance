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

        for (size_t i = 0; i < context->MAX_FRAMES_IN_FLIGHT; i++)
        {
            BufferCreateInfo bufCreateInfo;
            bufCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            bufCreateInfo.memoryHint = BufferMemoryHint::CpuToGpu;
            bufCreateInfo.cpuAccess = CpuAccessPattern::SequentialWrite;
            bufCreateInfo.persistentlyMapped = true;
            bufCreateInfo.size = bufferSize;

            flatDataBuffers[i] = context->createBuffer(bufCreateInfo);
        }
	}

	Material::~Material()
	{
	}

	
}