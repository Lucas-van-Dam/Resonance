#pragma once

#include "REON/Rendering/RenderContext.h"

#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>
#include "nlohmann/json.hpp"

namespace REON {
	struct QueueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		bool isComplete() {
			return graphicsFamily.has_value() && presentFamily.has_value();
		}
	};

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	struct VulkanVertex { // TEMPORARY
		glm::vec3 pos;
		glm::vec3 color;
		glm::vec2 texCoord;

		static VkVertexInputBindingDescription getBindingDescription() {
			VkVertexInputBindingDescription bindingDescription{};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(VulkanVertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescription;
		}

		static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
			std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[0].offset = offsetof(VulkanVertex, pos);

			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(VulkanVertex, color);

			attributeDescriptions[2].binding = 0;
			attributeDescriptions[2].location = 2;
			attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[2].offset = offsetof(VulkanVertex, texCoord);

			return attributeDescriptions;
		}
	};

	struct UniformBufferObject {
		alignas(16) glm::mat4 model;
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 projection;
	};

	class VulkanContext : public RenderContext
	{
	public:
		VulkanContext(GLFWwindow* window);
		void init() override;
		void startFrame() override;
		void endFrame() override;
		void cleanup() override;
		void resize() override;

		VkInstance getInstance() const { return m_Instance; }
		VkDevice getDevice() const { return m_Device; }
		VkPhysicalDevice getPhysicalDevice() const { return m_PhysicalDevice; }
		VkSurfaceKHR getSurface() const { return m_Surface; }
		VkSampleCountFlagBits getSampleCount() const { return m_MsaaSamples; }
		VkQueue getGraphicsQueue() const { return m_GraphicsQueue; }
		VkFormat getSwapChainFormat() const { return m_SwapChainImageFormat; }
		VkFence getCurrentFence() const { return m_InFlightFences[currentFrame]; }
		VkExtent2D getExtent() const { return m_SwapChainExtent; }
		std::vector<VkImageView> getSwapChainImageViews() const { return m_SwapChainImageViews; }
		int getCurrentFrame() const { return currentFrame; }
		uint32_t getCurrentImageIndex() const { return m_ImageIndex; }
		VkSemaphore getCurrentImageAvailableSemaphore() const { return m_ImageAvailableSemaphores[currentFrame]; }
		VkSemaphore getCurrentRenderFinishedSemaphore() const { return m_RenderFinishedSemaphores[currentFrame]; }
		VkSemaphore getCurrentGuiFinishedRenderingSemaphore() const { return m_GuiPresentedSemaphores[currentFrame]; }
		VmaAllocator getAllocator() const { return m_Allocator; }
		VkDescriptorPool getDescriptorPool() const { return m_DescriptorPool; }
		size_t getAmountOfSwapChainImages() const { return m_SwapChainImageViews.size(); }

		const int MAX_FRAMES_IN_FLIGHT = 2;

		QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) const;
		VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) const;
		void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkImage& image, VmaAllocation& imageAllocation) const;
		void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels) const;
		void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) const;

		void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationCreateFlags hostAccessFlags, VkBuffer& buffer, VmaAllocation& allocation) const;
		void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) const;
		VkShaderModule createShaderModule(const std::vector<char>& code) const;
		VkFormat findDepthFormat() const;
		void createCommandPool(VkCommandPool& commandPool, uint32_t queueFamilyIndex) const;
		void createCommandBuffers(std::vector<VkCommandBuffer>& commandBuffers, VkCommandPool commandPool, size_t amountOfBuffers) const;

	private:
		void createInstance();
		bool checkValidationLayerSupport();
		std::vector<const char*> getRequiredExtensions();
		void setupDebugMessenger();
		void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
		void pickPhysicalDevice();
		int rateDeviceSuitability(VkPhysicalDevice device);
		
		void createLogicalDevice();
		void createSurface();
		bool checkDeviceExtensions(VkPhysicalDevice device);
		SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
		VkSurfaceFormatKHR chooseSwapChainFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
		VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
		void createSwapChain();
		void createImageViews();
		void createCommandPool();
		void createSyncObjects();
		void recreateSwapChain();
		void cleanupSwapChain();
		uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
		void createVmaAllocator();
		void createDescriptorPool();
		VkCommandBuffer beginSingleTimeCommands() const;
		void endSingleTimeCommands(VkCommandBuffer commandBuffer) const;
		VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;
		bool hasStencilComponent(VkFormat format) const;
		void generateMipmaps(VkImage image, VkFormat format, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
		VkSampleCountFlagBits getMaxUsableSampleCount();

		VkResult createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
		void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

		static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData);

	private:
		GLFWwindow* m_WindowHandle;

		VkInstance m_Instance;
		VkDebugUtilsMessengerEXT m_DebugMessenger;
		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
		VkDevice m_Device;
		VmaAllocator m_Allocator;
		VkQueue m_GraphicsQueue;
		VkQueue m_PresentQueue;
		VkSurfaceKHR m_Surface;
		VkSwapchainKHR m_SwapChain;
		std::vector<VkImage> m_SwapChainImages;
		std::vector<VkImageView> m_SwapChainImageViews;
		VkFormat m_SwapChainImageFormat;
		VkExtent2D m_SwapChainExtent;
		VkRenderPass m_RenderPass;
		VkDescriptorSetLayout m_DescriptorSetLayout;
		VkDescriptorPool m_DescriptorPool;
		std::vector<VkDescriptorSet> m_DescriptorSets;
		std::vector<VkFramebuffer> m_SwapChainFramebuffers;
		VkCommandPool m_CommandPool;
		//std::vector<VkCommandBuffer> m_CommandBuffers;

		uint32_t m_ImageIndex;

		std::vector<VkSemaphore> m_ImageAvailableSemaphores;
		std::vector<VkSemaphore> m_RenderFinishedSemaphores;
		std::vector<VkSemaphore> m_GuiPresentedSemaphores;
		std::vector<VkFence> m_InFlightFences;

		uint32_t m_MipLevels;

		VkSampleCountFlagBits m_MsaaSamples = VK_SAMPLE_COUNT_1_BIT;

		bool framebufferResized = false;

		int currentFrame = 0;

		const std::vector<const char*> m_ValidationLayers = {
			"VK_LAYER_KHRONOS_validation"
		};

		const std::vector<const char*> m_DeviceExtensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
			VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME,
			VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME
			/*VK_NV_PER_STAGE_DESCRIPTOR_SET_EXTENSION_NAME*/
		};




#ifdef REON_DEBUG
		const bool m_EnableValidationLayers = true;
#else
		const bool m_EnableValidationLayers = false;
#endif
	};

}