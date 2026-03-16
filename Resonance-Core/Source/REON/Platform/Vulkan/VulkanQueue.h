#pragma once

#include <vulkan/vulkan.h>

namespace REON
{
class VulkanQueue
{
  public:
    VulkanQueue(){}
    ~VulkanQueue(){}

    void init(VkDevice device, VkSwapchainKHR swapChain, uint32_t queueFamily, uint32_t queueIndex);

    void destroy();

    uint32_t aquireNextImage();

    void submitSync(VkCommandBuffer cmdBuf);

    void submitAsync(VkCommandBuffer cmdBuf);
    
    void present(uint32_t imageIndex);

    void waitIdle();

    private:
    void createSemaphores();

    VkDevice m_Device = VK_NULL_HANDLE;
    VkSwapchainKHR m_SwapChain = VK_NULL_HANDLE;
    VkQueue m_Queue = VK_NULL_HANDLE;
    VkSemaphore m_renderCompleteSem;
    VkSemaphore m_presentCompleteSem;
};
}