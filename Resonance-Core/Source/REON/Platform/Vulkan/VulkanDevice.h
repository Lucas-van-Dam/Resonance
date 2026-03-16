#pragma once

#include <vector>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace REON
{
struct PhysicalDevice
{
    VkPhysicalDevice m_physDevice;
    VkPhysicalDeviceProperties m_devProps;
    std::vector<VkQueueFamilyProperties> m_qFamilyProps;
    std::vector<VkBool32> m_qSupportsPresent;
    std::vector<VkSurfaceFormatKHR> m_surfaceFormats;
    VkSurfaceCapabilitiesKHR m_surfaceCaps;
    VkPhysicalDeviceMemoryProperties m_memProps;
    std::vector<VkPresentModeKHR> m_presentModes;
    VkPhysicalDeviceFeatures m_features;
};

class VulkanPhysicalDevices
{

  public:
    VulkanPhysicalDevices() {}
    ~VulkanPhysicalDevices() {}

    void Init(const VkInstance& instance, const VkSurfaceKHR& surface);

    uint32_t SelectDevice(VkQueueFlags RequiredQueueType, bool SupportsPresent);

    void refreshDetails(VkSurfaceKHR surface);

    const PhysicalDevice& Selected() const;

    private:
    std::vector<PhysicalDevice> m_devices;

    int m_devIndex = -1;
};

} // namespace REON