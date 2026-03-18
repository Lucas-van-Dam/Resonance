#pragma once

#include <vector>
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>

namespace REON
{



class Device
{

};

class PhysicalDevice
{
  public:
    VkPhysicalDevice m_physDevice;
    VkPhysicalDeviceProperties2 m_coreDevProps;
    VkPhysicalDeviceVulkan11Properties m_v11DevProps;
    VkPhysicalDeviceVulkan12Properties m_v12DevProps;
    VkPhysicalDeviceVulkan13Properties m_v13DevProps;
    VkPhysicalDeviceVulkan14Properties m_v14DevProps;
    std::vector<VkQueueFamilyProperties> m_qFamilyProps;
    std::vector<VkBool32> m_qSupportsPresent;
    std::vector<VkSurfaceFormatKHR> m_surfaceFormats;
    VkSurfaceCapabilitiesKHR m_surfaceCaps;
    VkPhysicalDeviceMemoryProperties m_memProps;
    std::vector<VkPresentModeKHR> m_presentModes;
    VkPhysicalDeviceFeatures2 m_coreFeatures;
    VkPhysicalDeviceVulkan11Features m_v11Features;
    VkPhysicalDeviceVulkan12Features m_v12Features;
    VkPhysicalDeviceVulkan13Features m_v13Features;
    VkPhysicalDeviceVulkan14Features m_v14Features;
    bool hasVk14;
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