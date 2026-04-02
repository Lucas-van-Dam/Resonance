#include "reonpch.h"

#include "VulkanImage.h"
#include "VulkanContext.h"

namespace REON
{
VulkanImage::VulkanImage(VulkanImage&& other) noexcept
{
    moveFrom(std::move(other));
}

VulkanImage& VulkanImage::operator=(VulkanImage&& other) noexcept
{
    if (this != &other)
    {
        reset();
        moveFrom(std::move(other));
    }
    return *this;
}

VulkanImage::~VulkanImage()
{
    reset();
}

VulkanImage::VulkanImage(const VulkanContext* context, VkImage image, VkImageView view, VmaAllocation allocation,
                         ImageCreateInfo createInfo)
    : m_context(context), m_texture(image), m_textureView(view), m_textureAllocation(allocation),
      m_imageCreateInfo(createInfo), m_currentLayout(createInfo.initialLayout)
{
    
}

void VulkanImage::reset() {
    if (m_texture != VK_NULL_HANDLE)
    {
        vmaDestroyImage(m_context->getAllocator(), m_texture, m_textureAllocation);     
    }
    if (m_textureView != VK_NULL_HANDLE)
    {
        vkDestroyImageView(m_context->getDevice(), m_textureView, nullptr);
    }
}

void VulkanImage::moveFrom(VulkanImage&& other) 
{
    m_context = other.m_context;
    m_texture = other.m_texture;
    m_textureAllocation = other.m_textureAllocation;
    m_imageCreateInfo = other.m_imageCreateInfo;
    m_textureView = other.m_textureView;

    other.m_context = nullptr;
    other.m_texture = VK_NULL_HANDLE;
    other.m_textureAllocation = nullptr;
    other.m_textureView = VK_NULL_HANDLE;
}


} // namespace REON
