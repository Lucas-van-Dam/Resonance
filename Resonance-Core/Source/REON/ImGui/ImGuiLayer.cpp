#include "reonpch.h"
#include "ImGuiLayer.h"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"

#include "REON/Application.h"

#include <GLFW/glfw3.h>
#include "ImGuizmo.h"
#include <glm/ext/matrix_float4x4.hpp>
#include "REON/GameHierarchy/SceneManager.h"
#include "REON/GameHierarchy/Components/Transform.h"
#include <glm/gtc/type_ptr.hpp>

namespace REON {

	ImGuiLayer::ImGuiLayer(VulkanContext* context)
		: Layer("ImGuiLayer"), m_Context(context)
	{

	}

	ImGuiLayer::~ImGuiLayer()
	{

	}

	void ImGuiLayer::OnDetach()
	{
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
		
		OnCleanup();
	}

	void ImGuiLayer::OnAttach()
	{
		EventBus::Get().subscribe<ContextResizeEvent>(REON_BIND_EVENT_FN(ImGuiLayer::onResize));

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamePad;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

		ImGui::StyleColorsDark();

		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		Application& app = Application::Get();
		GLFWwindow* window = static_cast<GLFWwindow*>(app.GetWindow().GetNativeWindow());

		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = m_Context->getSwapChainFormat();
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkSubpassDescription subPassDesc{};
		subPassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subPassDesc.colorAttachmentCount = 1;
		subPassDesc.pColorAttachments = &colorAttachmentRef;

		VkRenderPassCreateInfo passCreateInfo{};
		passCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		passCreateInfo.attachmentCount = 1;
		passCreateInfo.pAttachments = &colorAttachment;
		passCreateInfo.subpassCount = 1;
		passCreateInfo.pSubpasses = &subPassDesc;
		passCreateInfo.dependencyCount = 1;
		passCreateInfo.pDependencies = &dependency;

		VkResult res = vkCreateRenderPass(m_Context->getDevice(), &passCreateInfo, nullptr, &m_RenderPass);
		REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to create render pass");

		ImGui_ImplVulkan_InitInfo initInfo{};
		initInfo.ApiVersion = VK_API_VERSION_1_3;
		initInfo.Instance = m_Context->getInstance();
		initInfo.PhysicalDevice = m_Context->getPhysicalDevice();
		initInfo.Device = m_Context->getDevice();
		initInfo.QueueFamily = m_Context->findQueueFamilies(m_Context->getPhysicalDevice()).graphicsFamily.value();
		initInfo.Queue = m_Context->getGraphicsQueue();
		initInfo.RenderPass = m_RenderPass;
		initInfo.MinImageCount = static_cast<uint32_t>(m_Context->getSwapChainImageViews().size());
		initInfo.ImageCount = static_cast<uint32_t>(m_Context->getSwapChainImageViews().size());
		initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		initInfo.DescriptorPoolSize = IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE + 3;

		ImGui_ImplGlfw_InitForVulkan(window, true);
		ImGui_ImplVulkan_Init(&initInfo);

		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = m_Context->findQueueFamilies(m_Context->getPhysicalDevice()).graphicsFamily.value();
		res = vkCreateCommandPool(m_Context->getDevice(), &poolInfo, nullptr, &m_CommandPool);
		REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to create command pool");

		m_CommandBuffers.resize(m_Context->MAX_FRAMES_IN_FLIGHT);
		VkCommandBufferAllocateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		bufferInfo.commandPool = m_CommandPool;
		bufferInfo.commandBufferCount = static_cast<uint32_t>(m_CommandBuffers.size());
		res = vkAllocateCommandBuffers(m_Context->getDevice(), &bufferInfo, m_CommandBuffers.data());
		REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to allocate command buffer");

		createFrameBuffers();

		ImGui_ImplVulkan_CreateFontsTexture();
	}



	void ImGuiLayer::Begin()
	{
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}

	void ImGuiLayer::End()
	{
		ImGuiIO& io = ImGui::GetIO();
		Application& app = Application::Get();
		io.DisplaySize = ImVec2((float)app.GetWindow().GetWidth(), (float)app.GetWindow().GetHeight());

		ImGui::Render();

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		VkResult res = vkBeginCommandBuffer(m_CommandBuffers[m_Context->getCurrentFrame()], &beginInfo);
		REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to begin command buffer")

		VkRenderPassBeginInfo renderPassinfo{};
		renderPassinfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassinfo.renderPass = m_RenderPass;
		renderPassinfo.framebuffer = m_Framebuffers[m_Context->getCurrentImageIndex()];
		renderPassinfo.renderArea.extent.width = m_Context->getExtent().width;
		renderPassinfo.renderArea.extent.height = m_Context->getExtent().height;
		renderPassinfo.clearValueCount = 1;
		VkClearValue clearValue = { 0.0f, 0.0f, 0.0f, 1.0f };
		renderPassinfo.pClearValues = &clearValue;

		vkCmdBeginRenderPass(m_CommandBuffers[m_Context->getCurrentFrame()], &renderPassinfo, VK_SUBPASS_CONTENTS_INLINE);

		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_CommandBuffers[m_Context->getCurrentFrame()]);

		vkCmdEndRenderPass(m_CommandBuffers[m_Context->getCurrentFrame()]);

		res = vkEndCommandBuffer(m_CommandBuffers[m_Context->getCurrentFrame()]);
		REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to end command buffer");

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { m_Context->getCurrentRenderFinishedSemaphore() };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_CommandBuffers[m_Context->getCurrentFrame()];

		VkSemaphore signalSemaphores[] = { m_Context->getCurrentGuiFinishedRenderingSemaphore()};
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		res = vkQueueSubmit(m_Context->getGraphicsQueue(), 1, &submitInfo, m_Context->getCurrentFence());
		REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to submit draw command buffer");

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) 
		{
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}
	}
	
	void ImGuiLayer::OnImGuiRender()
	{
		
	}

	void ImGuiLayer::OnCleanup()
	{
		for (size_t i = 0; i < m_Context->getAmountOfSwapChainImages(); i++) {
			vkDestroyFramebuffer(m_Context->getDevice(), m_Framebuffers[i], nullptr);
		}

		vkDestroyRenderPass(m_Context->getDevice(), m_RenderPass, nullptr);

		vkDestroyCommandPool(m_Context->getDevice(), m_CommandPool, nullptr);
	}

	void ImGuiLayer::createFrameBuffers()
	{
		VkImageView attachment[1];
		VkFramebufferCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		info.renderPass = m_RenderPass;
		info.attachmentCount = 1;
		info.pAttachments = attachment;
		info.width = m_Context->getExtent().width;
		info.height = m_Context->getExtent().height;
		info.layers = 1;
		m_Framebuffers.resize(m_Context->getSwapChainImageViews().size());
		for (uint32_t i = 0; i < m_Context->getSwapChainImageViews().size(); i++)
		{
			attachment[0] = m_Context->getSwapChainImageViews()[i];
			VkResult res = vkCreateFramebuffer(m_Context->getDevice(), &info, nullptr, &m_Framebuffers[i]);
			REON_CORE_ASSERT(res == VK_SUCCESS, "Failed to create imgui framebuffer number {}", i);
		}
	}

	void ImGuiLayer::onResize(const ContextResizeEvent& event)
	{
		vkDeviceWaitIdle(m_Context->getDevice());
		for (size_t i = 0; i < m_Context->getAmountOfSwapChainImages(); i++) {
			vkDestroyFramebuffer(m_Context->getDevice(), m_Framebuffers[i], nullptr);
		}

		createFrameBuffers();
	}
}