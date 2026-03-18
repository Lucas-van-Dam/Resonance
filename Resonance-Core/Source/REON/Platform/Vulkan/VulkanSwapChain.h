#pragma once
#include "VulkanContext.h"

#include <vulkan/vulkan.h>

namespace REON
{
 struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
 };

class VulkanSwapChain
{
  public:
    VulkanSwapChain(VulkanContext& ctx, VkDevice& dev);
    ~VulkanSwapChain();

    void init();
    void cleanup();
    void recreate(uint32_t w, uint32_t h);

    VkExtent2D extent() const;

  private:
    VulkanContext& m_Context;
    VkDevice& m_device;
    VkSurfaceKHR m_Surface;
    VkSwapchainKHR m_SwapChain = VK_NULL_HANDLE;
    VkExtent2D m_Extent;
    std::vector<VkImage> m_Images;
    std::vector<VkImageView> m_Views;
    std::vector<VkSemaphore> m_releaseSemaphores;
    VkSurfaceFormat2KHR m_swapchainSurfaceFormat;
    VkPresentModeKHR m_activePresentMode;


    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
    VkSurfaceFormatKHR chooseSwapChainFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
};

} // namespace REON