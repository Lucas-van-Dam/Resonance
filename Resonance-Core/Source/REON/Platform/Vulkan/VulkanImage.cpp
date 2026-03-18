#include "reonpch.h"

#include "VulkanImage.h"
#include "VulkanContext.h"

namespace REON
{
VulkanImage::~VulkanImage() 
{
    vmaDestroyImage(m_context->getAllocator(), m_texture, m_textureAllocation);
    vkDestroyImageView(m_context->getDevice(), m_textureView, nullptr);
}

VulkanImage::VulkanImage(const VulkanContext* context, VkImage image, VkImageView view, VmaAllocation allocation,
                         ImageCreateInfo createInfo)
    : m_context(context), m_texture(image), m_textureView(view), m_textureAllocation(allocation),
      m_imageCreateInfo(createInfo), m_currentLayout(createInfo.initialLayout)
{
    
}


} // namespace REON
