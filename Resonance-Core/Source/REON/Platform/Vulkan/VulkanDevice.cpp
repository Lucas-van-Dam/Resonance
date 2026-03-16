#include "reonpch.h"

#include "VulkanDevice.h"

namespace REON
{
void VulkanPhysicalDevices::Init(const VkInstance& instance, const VkSurfaceKHR& surface)
{
    uint32_t numDevices = 0;

    vkEnumeratePhysicalDevices(instance, &numDevices, VK_NULL_HANDLE);

    REON_CORE_ASSERT(numDevices > 0, "Failed to find GPUs with Vulkan support");

    m_devices.resize(numDevices);

    std::vector<VkPhysicalDevice> devices(numDevices);

    vkEnumeratePhysicalDevices(instance, &numDevices, devices.data());

    std::multimap<int, VkPhysicalDevice> candidates;

    for (uint32_t i = 0; i < numDevices; ++i)
    {
        VkPhysicalDevice physDev = devices[i];
        m_devices[i].m_physDevice = physDev;

        vkGetPhysicalDeviceProperties(physDev, &m_devices[i].m_devProps);

        uint32_t numQFamilies = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physDev, &numQFamilies, VK_NULL_HANDLE);

        m_devices[i].m_qFamilyProps.resize(numQFamilies);
        m_devices[i].m_qSupportsPresent.resize(numQFamilies);

        vkGetPhysicalDeviceQueueFamilyProperties(physDev, &numQFamilies, m_devices[i].m_qFamilyProps.data());

        for (uint32_t q = 0; q < numQFamilies; ++q)
        {
            const VkQueueFamilyProperties& qFamilyProp = m_devices[i].m_qFamilyProps[q];

            vkGetPhysicalDeviceSurfaceSupportKHR(physDev, q, surface, &(m_devices[i].m_qSupportsPresent[q]));
        }

        uint32_t numFormats = 0;

        vkGetPhysicalDeviceSurfaceFormatsKHR(physDev, surface, &numFormats, VK_NULL_HANDLE);

        m_devices[i].m_surfaceFormats.resize(numFormats);

        vkGetPhysicalDeviceSurfaceFormatsKHR(physDev, surface, &numFormats, m_devices[i].m_surfaceFormats.data());

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physDev, surface, &m_devices[i].m_surfaceCaps);

        uint32_t numPresentModes = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physDev, surface, &numPresentModes, VK_NULL_HANDLE);

        REON_CORE_ASSERT(numPresentModes != 0, "No presentmodes available for device");

        m_devices[i].m_presentModes.resize(numPresentModes);

        vkGetPhysicalDeviceSurfacePresentModesKHR(physDev, surface, &numPresentModes,
                                                  m_devices[i].m_presentModes.data());

        vkGetPhysicalDeviceMemoryProperties(physDev, &(m_devices[i].m_memProps));

        vkGetPhysicalDeviceFeatures(m_devices[i].m_physDevice, &m_devices[i].m_features);
    }
}

uint32_t VulkanPhysicalDevices::SelectDevice(VkQueueFlags requiredQueueType, bool supportsPresent)
{
    for (uint32_t i = 0; i < m_devices.size(); ++i)
    {
        for (uint32_t j = 0; j < m_devices[i].m_qFamilyProps.size(); ++j)
        {
            const VkQueueFamilyProperties& qFamilyProp = m_devices[i].m_qFamilyProps[j];

            if ((qFamilyProp.queueFlags & requiredQueueType) && ((bool)m_devices[i].m_qSupportsPresent[j] == supportsPresent)) {
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
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_devices[m_devIndex].m_physDevice, surface, &m_devices[m_devIndex].m_surfaceCaps);
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
