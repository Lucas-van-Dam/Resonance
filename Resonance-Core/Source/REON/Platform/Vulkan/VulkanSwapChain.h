#pragma once
#include <vulkan/vulkan.h>
#include "VulkanContext.h"

namespace REON {
	//struct SwapChainSupportDetails {
	//	VkSurfaceCapabilitiesKHR capabilities;
	//	std::vector<VkSurfaceFormatKHR> formats;
	//	std::vector<VkPresentModeKHR> presentModes;
	//};

	class VulkanSwapChain
	{
	public:
		VulkanSwapChain(VulkanContext& ctx);
		~VulkanSwapChain();

		void init();
		void cleanup();
		void recreate(uint32_t w, uint32_t h);

		VkExtent2D extent() const;

	private:
		VulkanContext& m_Context;
		VkSwapchainKHR m_SwapChain = VK_NULL_HANDLE;
		VkFormat m_Format;
		VkExtent2D m_Extent;
		std::vector<VkImage> m_Images;
		std::vector<VkImageView> m_Views;

		SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
		VkSurfaceFormatKHR chooseSwapChainFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
		VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	};

}