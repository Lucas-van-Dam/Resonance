#include "reonpch.h"

#include "VulkanBuffer.h"

#include "VulkanContext.h"

namespace REON
{


VulkanBuffer::VulkanBuffer(const VulkanContext* device, VkBuffer buffer, VmaAllocation allocation,
                           VmaAllocationInfo allocInfo, BufferCreateInfo usage)
    : m_context(device), m_buffer(buffer), m_allocation(allocation), m_allocInfo(allocInfo),
      m_createInfo(usage)
{
}

VulkanBuffer::~VulkanBuffer() 
{
    Reset();
}

void VulkanBuffer::Write(const void* data, size_t size)
{
    if (m_createInfo.persistentlyMapped && m_allocInfo.pMappedData != nullptr)
    {
        memcpy(m_allocInfo.pMappedData, data, size);
    }
    else
    {
        vmaCopyMemoryToAllocation(m_context->getAllocator(), data, m_allocation, 0, size);
    }
}

void VulkanBuffer::Reset() {
    if (m_buffer != VK_NULL_HANDLE) {
        vmaDestroyBuffer(m_context->getAllocator(), m_buffer, m_allocation);
        m_buffer = VK_NULL_HANDLE;
        m_allocation = nullptr;
    }
}

void VulkanBuffer::MoveFrom(VulkanBuffer&& other) {
    m_context = other.m_context;
    m_buffer = other.m_buffer;
    m_allocation = other.m_allocation;
    m_allocInfo = other.m_allocInfo;
    m_createInfo = other.m_createInfo;

    other.m_context = nullptr;
    other.m_buffer = VK_NULL_HANDLE;
    other.m_allocation = nullptr;
}

VulkanBuffer::VulkanBuffer(VulkanBuffer&& other) noexcept
{
    MoveFrom(std::move(other));
}

VulkanBuffer& VulkanBuffer::operator = (VulkanBuffer && other) noexcept
{
    if (this != &other)
    {
        Reset();
        MoveFrom(std::move(other));
    }
    return *this;
}

} // namespace REON
