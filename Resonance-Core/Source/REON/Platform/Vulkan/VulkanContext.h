#pragma once

#include "REON/Rendering/RenderContext.h"

#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

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

	struct Vertex { // TEMPORARY
		glm::vec2 pos;
		glm::vec3 color;

		static VkVertexInputBindingDescription getBindingDescription() {
			VkVertexInputBindingDescription bindingDescription{};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(Vertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescription;
		}

		static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
			std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[0].offset = offsetof(Vertex, pos);

			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(Vertex, color);

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
		void swapBuffers() override;
		void cleanup() override;
		void resize() override;

	private:
		void createInstance();
		bool checkValidationLayerSupport();
		std::vector<const char*> getRequiredExtensions();
		void setupDebugMessenger();
		void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
		void pickPhysicalDevice();
		int rateDeviceSuitability(VkPhysicalDevice device);
		QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
		void createLogicalDevice();
		void createSurface();
		bool checkDeviceExtensions(VkPhysicalDevice device);
		SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
		VkSurfaceFormatKHR chooseSwapChainFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
		VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
		void createSwapChain();
		void createImageViews();
		void createGraphicsPipeline();
		VkShaderModule createShaderModule(const std::vector<char>& code);
		void createRenderPass();
		void createFramebuffers();
		void createCommandPool();
		void createCommandBuffers();
		void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t currentFrame);
		void createSyncObjects();
		void recreateSwapChain();
		void cleanupSwapChain();
		void createVertexBuffer();
		void createIndexBuffer();
		uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
		void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationCreateFlags hostAccessFlags, VkBuffer& buffer, VmaAllocation& allocation);
		void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
		void createVmaAllocator();
		void createDescriptorSetLayout();
		void createUniformBuffers();
		void updateUniformBuffer(uint32_t currentImage);
		void createDescriptorPool();
		void createDescriptorSets();

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
		VkPipelineLayout m_PipelineLayout;
		VkPipeline m_GraphicsPipeline;
		std::vector<VkFramebuffer> m_SwapChainFramebuffers;
		VkCommandPool m_CommandPool;
		std::vector<VkCommandBuffer> m_CommandBuffers;

		std::vector<VkSemaphore> m_ImageAvailableSemaphores;
		std::vector<VkSemaphore> m_RenderFinishedSemaphores;
		std::vector<VkFence> m_InFlightFences;

		VkBuffer m_VertexBuffer;
		VmaAllocation m_VertexBufferAllocation;
		VkBuffer m_IndexBuffer;
		VmaAllocation m_IndexBufferAllocation;
		
		std::vector<VkBuffer> m_UniformBuffers;
		std::vector<VmaAllocation> m_UniformBufferAllocations;
		std::vector<void*> m_UniformBuffersMapped;

		bool framebufferResized = false;

		const std::vector<const char*> m_ValidationLayers = {
			"VK_LAYER_KHRONOS_validation"
		};

		const std::vector<const char*> m_DeviceExtensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		};

		const int MAX_FRAMES_IN_FLIGHT = 2;

		const std::array<Vertex, 4> m_Vertices = {
					Vertex(glm::vec2(-0.5f, -0.5f), glm::vec3(1.0f, 0.0f, 0.0f)),
					Vertex(glm::vec2(0.5f, -0.5f), glm::vec3(0.0f, 1.0f, 0.0f)),
					Vertex(glm::vec2(0.5f, 0.5f), glm::vec3(0.0f, 0.0f, 1.0f)),
					Vertex(glm::vec2(-0.5f, 0.5f), glm::vec3(1.0f, 1.0f, 1.0f)),
		};
		const std::array<uint32_t, 6> m_Indices = {
			0,1,2,2,3,0
		};

#ifdef REON_DEBUG
		const bool m_EnableValidationLayers = true;
#else
		const bool m_EnableValidationLayers = false;
#endif
	};

}