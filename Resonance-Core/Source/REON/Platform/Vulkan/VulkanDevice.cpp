#include "reonpch.h"

#include "VulkanDevice.h"

namespace REON
{
void VulkanPhysicalDevices::Init(const VkInstance& instance, const VkSurfaceKHR& surface)
{
    uint32_t numDevices = 0;

    vkEnumeratePhysicalDevices(instance, &numDevices, VK_NULL_HANDLE);

    REON_CORE_ASSERT(numDevices > 0, "Failed to find GPUs with Vulkan support");

    std::vector<VkPhysicalDevice> devices(numDevices);

    vkEnumeratePhysicalDevices(instance, &numDevices, devices.data());

    std::multimap<int, VkPhysicalDevice> candidates;

    for (uint32_t i = 0; i < numDevices; ++i)
    {
        VkPhysicalDevice physDev = devices[i];

        PhysicalDevice dev;
        dev.m_physDevice = physDev;

        dev.m_coreDevProps = {};
        dev.m_v11DevProps = {};
        dev.m_v12DevProps = {};
        dev.m_v13DevProps = {};

        dev.m_coreDevProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        dev.m_v11DevProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES;
        dev.m_v12DevProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES;
        dev.m_v13DevProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES;
        dev.m_coreDevProps.pNext = &dev.m_v11DevProps;
        dev.m_v11DevProps.pNext = &dev.m_v12DevProps;
        dev.m_v12DevProps.pNext = &dev.m_v13DevProps;

        vkGetPhysicalDeviceProperties2(physDev, &dev.m_coreDevProps);

        if (dev.m_coreDevProps.properties.apiVersion >= VK_API_VERSION_1_4)
        {
            dev.m_v14DevProps = {};
            dev.hasVk14 = true;
            dev.m_v14DevProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_PROPERTIES;
            dev.m_v13DevProps.pNext = &dev.m_v14DevProps;
            dev.m_v14DevProps.pNext = nullptr;
        }
        else
        {
            if (dev.m_coreDevProps.properties.apiVersion < VK_API_VERSION_1_3)
            {
                REON_CORE_INFO("Device {} does not meet minimum api version (1.3.x)",
                               dev.m_coreDevProps.properties.deviceName);
                break;
            }
        }

        vkGetPhysicalDeviceProperties2(physDev, &dev.m_coreDevProps);

        uint32_t numQFamilies = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physDev, &numQFamilies, VK_NULL_HANDLE);

        dev.m_qFamilyProps.resize(numQFamilies);
        dev.m_qSupportsPresent.resize(numQFamilies);

        vkGetPhysicalDeviceQueueFamilyProperties(physDev, &numQFamilies, dev.m_qFamilyProps.data());

        for (uint32_t q = 0; q < numQFamilies; ++q)
        {
            const VkQueueFamilyProperties& qFamilyProp = dev.m_qFamilyProps[q];

            vkGetPhysicalDeviceSurfaceSupportKHR(physDev, q, surface, &(dev.m_qSupportsPresent[q]));
        }

        uint32_t numFormats = 0;

        vkGetPhysicalDeviceSurfaceFormatsKHR(physDev, surface, &numFormats, VK_NULL_HANDLE);

        dev.m_surfaceFormats.resize(numFormats);

        vkGetPhysicalDeviceSurfaceFormatsKHR(physDev, surface, &numFormats, dev.m_surfaceFormats.data());

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physDev, surface, &dev.m_surfaceCaps);

        uint32_t numPresentModes = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physDev, surface, &numPresentModes, VK_NULL_HANDLE);

        REON_CORE_ASSERT(numPresentModes != 0, "No presentmodes available for device");

        dev.m_presentModes.resize(numPresentModes);

        vkGetPhysicalDeviceSurfacePresentModesKHR(physDev, surface, &numPresentModes, dev.m_presentModes.data());

        vkGetPhysicalDeviceMemoryProperties(physDev, &(dev.m_memProps));

        dev.m_coreFeatures = {};
        dev.m_v11Features = {};
        dev.m_v12Features = {};
        dev.m_v13Features = {};
        dev.m_v14Features = {};

        dev.m_coreFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        dev.m_v11Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
        dev.m_v12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        dev.m_v13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;

        dev.m_coreFeatures.pNext = &dev.m_v11Features;
        dev.m_v11Features.pNext = &dev.m_v12Features;
        dev.m_v12Features.pNext = &dev.m_v13Features;

        if (dev.hasVk14)
        {
            dev.m_v14Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES;
            dev.m_v13Features.pNext = &dev.m_v14Features;
            dev.m_v14Features.pNext = nullptr;
        }

        vkGetPhysicalDeviceFeatures2(dev.m_physDevice, &dev.m_coreFeatures);

        m_devices.push_back(dev);
    }

    if (m_devices.empty())
    {
        REON_CORE_CRITICAL("No gpu suitable gpu found (needs vulkan API 1.3 or higher)");
    }
}

uint32_t VulkanPhysicalDevices::SelectDevice(VkQueueFlags requiredQueueType, bool supportsPresent)
{
    for (uint32_t i = 0; i < m_devices.size(); ++i)
    {
        for (uint32_t j = 0; j < m_devices[i].m_qFamilyProps.size(); ++j)
        {
            const VkQueueFamilyProperties& qFamilyProp = m_devices[i].m_qFamilyProps[j];

            if ((qFamilyProp.queueFlags & requiredQueueType) &&
                ((bool)m_devices[i].m_qSupportsPresent[j] == supportsPresent))
            {
                m_devIndex = i;
                int queueFamily = j;
                return queueFamily;
            }
        }
    }

    REON_CORE_ERROR("Required queue type {} and supports present {} not found", requiredQueueType, supportsPresent);

    return 0;
}

void VulkanPhysicalDevices::refreshDetails(VkSurfaceKHR surface)
{
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_devices[m_devIndex].m_physDevice, surface,
                                              &m_devices[m_devIndex].m_surfaceCaps);
}

const PhysicalDevice& VulkanPhysicalDevices::Selected() const
{
    if (m_devIndex < 0)
    {
        REON_CORE_ERROR("A physical device has not been selected");
        return {};
    }

    return m_devices[m_devIndex];
}

} // namespace REON
