#include "reonpch.h"
#include "VulkanQueue.h"


namespace REON {
void VulkanQueue::init(VkDevice device, VkSwapchainKHR swapChain, uint32_t queueFamily,
                                    uint32_t queueIndex)
{
    m_Device = device;
    m_SwapChain = swapChain;

    vkGetDeviceQueue(device, queueFamily, queueIndex, &m_Queue);

    createSemaphores();
}

void VulkanQueue::destroy() {
    vkDestroySemaphore(m_Device, m_renderCompleteSem, VK_NULL_HANDLE);
    vkDestroySemaphore(m_Device, m_presentCompleteSem, VK_NULL_HANDLE);
}

void VulkanQueue::createSemaphores() {
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    vkCreateSemaphore(m_Device, &semaphoreInfo, VK_NULL_HANDLE, &m_renderCompleteSem);
    vkCreateSemaphore(m_Device, &semaphoreInfo, VK_NULL_HANDLE, &m_presentCompleteSem);
}

uint32_t VulkanQueue::aquireNextImage() {
    uint32_t imageIndex = 0;
    VkResult res = vkAcquireNextImageKHR(m_Device, m_SwapChain, UINT64_MAX, m_presentCompleteSem,
                                VK_NULL_HANDLE, &imageIndex);
    return imageIndex;
}

void VulkanQueue::submitSync(VkCommandBuffer cmdBuf) {
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuf;

    vkQueueSubmit(m_Queue, 1, &submitInfo, VK_NULL_HANDLE);
}

void VulkanQueue::submitAsync(VkCommandBuffer cmdBuf) {
    VkPipelineStageFlags waitFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &m_presentCompleteSem;
    submitInfo.pWaitDstStageMask = &waitFlags;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuf;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &m_renderCompleteSem;

    vkQueueSubmit(m_Queue, 1, &submitInfo, VK_NULL_HANDLE);
}

void VulkanQueue::present(uint32_t imageIndex) {
    VkPresentInfoKHR presentInfo
    {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &m_renderCompleteSem,
        .swapchainCount = 1,
        .pSwapchains = &m_SwapChain,
        .pImageIndices = &imageIndex
    };

    vkQueuePresentKHR(m_Queue, &presentInfo);

}
}