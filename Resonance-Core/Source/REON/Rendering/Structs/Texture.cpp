#include "reonpch.h"
#include "Texture.h"
#include "REON/Application.h"
#include "REON/Platform/Vulkan/VulkanContext.h"

namespace REON {

	void Texture::Load(const std::string& filePath, std::any metadata)
	{
		const VulkanContext* context = static_cast<const VulkanContext*>(Application::Get().GetRenderContext());

		int texWidth, texHeight, texChannels;
		stbi_uc* pixels = stbi_load("Assets/Textures/texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		VkDeviceSize imageSize = texWidth * texHeight * 4;

		REON_CORE_ASSERT(pixels, "Failed to load texture image");

		//m_MipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

		VkBuffer stagingBuffer;
		VmaAllocation stagingBufferAllocation;

		context->createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, stagingBuffer, stagingBufferAllocation);

		void* data;
		vmaMapMemory(context->getAllocator(), stagingBufferAllocation, &data);
		memcpy(data, pixels, static_cast<size_t>(imageSize));
		vmaUnmapMemory(context->getAllocator(), stagingBufferAllocation);

		stbi_image_free(pixels);

		context->createImage(texWidth, texHeight, 1, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, m_Texture, m_TextureAllocation);

		context->transitionImageLayout(m_Texture, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1);
		context->copyBufferToImage(stagingBuffer, m_Texture, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

		//generateMipmaps(m_TextureImage, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, m_MipLevels);
		context->transitionImageLayout(m_Texture, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);

		vkDestroyBuffer(context->getDevice(), stagingBuffer, nullptr);
		vmaFreeMemory(context->getAllocator(), stagingBufferAllocation);

		m_TextureView = context->createImageView(m_Texture, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, 1);

		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;


		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(context->getPhysicalDevice(), &properties); // TODO: IF I USE THIS MORE STORE IT AT THE START AT CREATION

		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
		samplerInfo.mipLodBias = 0.0f;

		VkResult res = vkCreateSampler(context->getDevice(), &samplerInfo, nullptr, &m_TextureSampler);
		REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to create texture sampler");
		m_Path = filePath;
	}

	void Texture::Unload()
	{
		const VulkanContext* context = static_cast<const VulkanContext*>(Application::Get().GetRenderContext());

		vkDestroyImage(context->getDevice(), m_Texture, nullptr);
		vmaFreeMemory(context->getAllocator(), m_TextureAllocation);
		vkDestroyImageView(context->getDevice(), m_TextureView, nullptr);
		vkDestroySampler(context->getDevice(), m_TextureSampler, nullptr);
	}

	VkImageView Texture::getTextureView() const
	{
		return m_TextureView;
	}

	VkSampler Texture::getSampler() const
	{
		return m_TextureSampler;
	}
}