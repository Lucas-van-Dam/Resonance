#pragma once

#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace REON
{

enum class BufferMemoryHint
{
    GpuOnly,
    CpuToGpu,
    GpuToCpu,
    CpuOnly,
};

enum class CpuAccessPattern
{
    None,
    SequentialWrite,
    RandomReadWrite,
};

struct BufferCreateInfo
{
    VkBufferUsageFlags usage = 0;
    VkDeviceSize size = 0;
    BufferMemoryHint memoryHint = BufferMemoryHint::GpuOnly;
    CpuAccessPattern cpuAccess = CpuAccessPattern::None;
    bool persistentlyMapped = false;
    bool allowTransferFallback = false;
};

class VulkanContext;
class VulkanBuffer
{
  public:
    VulkanBuffer() = default;
    VulkanBuffer(VulkanBuffer&&) noexcept;
    VulkanBuffer& operator=(VulkanBuffer&&) noexcept;
    VulkanBuffer(const VulkanBuffer&) = delete;
    VulkanBuffer& operator=(const VulkanBuffer&) = delete;
    ~VulkanBuffer();

    VkBuffer GetVkBuffer() const
    {
        return m_buffer;
    }
    size_t GetSize() const
    {
        return m_createInfo.size;
    }

    void Write(const void* data, size_t size);

  private:
    friend class VulkanContext;
    VulkanBuffer(const VulkanContext* device, VkBuffer buffer, VmaAllocation allocation, VmaAllocationInfo allocInfo, BufferCreateInfo usage);

    void Reset();

    void MoveFrom(VulkanBuffer&& other);

    const VulkanContext* m_context = nullptr;
    VkBuffer m_buffer = VK_NULL_HANDLE;
    VmaAllocation m_allocation = nullptr;
    VmaAllocationInfo m_allocInfo{};
    BufferCreateInfo m_createInfo{};
};
} // namespace REON