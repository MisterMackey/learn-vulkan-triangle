#include <cstddef>
#include <cstdint>
#include <exception>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>
// below define and include tells the included header to also include vulkan deps
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "debugshit.hpp"
#include "p_device.hpp"
#include "presentation.hpp"
#include "requirement.hpp"
#include "shaderLoading.hpp"
#include <algorithm>
#include <cstdlib>
#include <glm/glm.hpp>
#include <iostream>
#include <stdexcept>
#include <string.h>
#include <vector>
#include <array>

const int MAX_FRAMES_IN_FLIGHT = 2;
const int windowHeight = 800;
const int windowWidth = 600;

struct Vertex {
	glm::vec2 pos;
	glm::vec3 color;

	static VkVertexInputBindingDescription getBindingDescription()
	{
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions()
	{
		std::array<VkVertexInputAttributeDescription, 2> attributedescriptions{};
		attributedescriptions[0].binding = 0;
		attributedescriptions[0].location = 0;
		attributedescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
		attributedescriptions[0].offset = offsetof(Vertex, pos);

		attributedescriptions[1].binding = 0;
		attributedescriptions[1].location = 1;
		attributedescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributedescriptions[1].offset = offsetof(Vertex, color);
		return attributedescriptions;
	}
};

// clang-format off
const std::vector<Vertex> vertices = {
	{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
	{{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
	{{-0.0f, 0.5f}, {0.0f, 0.0f, 1.0f}}
};
// clang-format on

class TriangleApp
{
      public:
	void run()
	{
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	}

      private:
	GLFWwindow *window;
	VkInstance vkInstance;
	VkDevice device; // logical device
	VkDebugUtilsMessengerEXT debugMessenger;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkSurfaceKHR surface;
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	trianglePresentation::swapchainInformation swapchainInfo;
	std::vector<VkImageView> swapChainImageViews;
	std::vector<VkFramebuffer> swapChainFramebuffers;
	VkRenderPass renderPass;
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;
	VkCommandPool commandPool;
	uint32_t currentFrame = 0;

	std::vector<VkCommandBuffer> commandBuffers;
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;

	bool framebufferResized = false;

	void initWindow(void)
	{
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		window = glfwCreateWindow(windowHeight, windowWidth, "Vulkanerino", nullptr, nullptr);
		glfwSetWindowUserPointer(window, this);
		glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
	}
	static void framebufferResizeCallback(GLFWwindow *window, int width, int height)
	{
		auto app = reinterpret_cast<TriangleApp *>(glfwGetWindowUserPointer(window));
		app->framebufferResized = true;
	}
	void initVulkan(void)
	{
		createInstance();
		setupDebugMessenger();
		trianglePresentation::createSurface(vkInstance, window, &surface);
		p_device::pickPhysicalDevice(&physicalDevice, vkInstance, surface);
		if (physicalDevice == VK_NULL_HANDLE)
			throw std::runtime_error("failed to find a suitable GPU");
		p_device::createLogicalDevice(&device, physicalDevice, &graphicsQueue, &presentQueue, surface);
		trianglePresentation::createSwapchain(physicalDevice, device, surface, window, swapchainInfo);
		createImageViews();
		// im giving up on splitting all this shit up into files. I don't know enough to properly factor this shit anyway.
		// so im just following the tutorial now
		createRenderPass();
		createGraphicsPipeline();
		createFramebuffers();
		createCommandPool();
		createCommandBuffers();
		createSyncObjects();
	}
	void mainLoop(void)
	{
		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();
			drawFrame();
		}
		vkDeviceWaitIdle(device);
	}
	void cleanup(void)
	{
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {

			vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
			vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
			vkDestroyFence(device, inFlightFences[i], nullptr);
		}
		vkDestroyCommandPool(device, commandPool, nullptr);
		cleanupSwapChain();
		vkDestroyPipeline(device, graphicsPipeline, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyRenderPass(device, renderPass, nullptr);
		vkDestroyDevice(device, nullptr);
		if (enableValidationLayers) {
			debugshit::destroyDebugUtilsMesssengerExt(vkInstance, debugMessenger, nullptr);
		}
		vkDestroySurfaceKHR(vkInstance, surface, nullptr);
		vkDestroyInstance(vkInstance, nullptr);
		glfwDestroyWindow(window);
		glfwTerminate();
	}
	void setupDebugMessenger(void)
	{
		if (!enableValidationLayers)
			return;

		if (debugshit::CreateDebugUtilsMessengerEXT(vkInstance, nullptr, &debugMessenger) != VK_SUCCESS)
			throw std::runtime_error("Failed to set up debug messenger");
	}
	void createInstance(void)
	{
		if (enableValidationLayers && !trequirement::checkValidationLayerSupport()) {
			throw std::runtime_error("validation layer fucky wucky");
		}
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Hello Triangle";
		appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo createInfo{};
		// basic info
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		auto required_extensions = trequirement::getRequiredExtensions();

		trequirement::verifyRequiredExtensionsPresent(required_extensions.data(), static_cast<uint32_t>(required_extensions.size()));

		createInfo.enabledExtensionCount = static_cast<uint32_t>(required_extensions.size());
		createInfo.ppEnabledExtensionNames = required_extensions.data();

		// which global validation layers to enable
		// this should sit in trequirement now

		// this variable is defined outside the if block to make sure its not destroyed before the vkcreateinstance call
		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		if (enableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(trequirement::validationLayers.size());
			createInfo.ppEnabledLayerNames = trequirement::validationLayers.data();
			debugshit::populateDebugMessengerStruct(debugCreateInfo);
			createInfo.pNext = &debugCreateInfo;
		} else {
			createInfo.enabledLayerCount = 0;
			createInfo.pNext = nullptr;
		}
		VkResult result = vkCreateInstance(&createInfo, nullptr, &vkInstance);
		if (result != VK_SUCCESS)
			throw std::runtime_error("Creating instance went fucked\n");
	}

	void createImageViews(void)
	{
		swapChainImageViews.resize(swapchainInfo.swapchainImages.size());
		for (size_t i = 0; i < swapchainInfo.swapchainImages.size(); ++i) {
			VkImageViewCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = swapchainInfo.swapchainImages[i];
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = swapchainInfo.swapchainImageFormat;
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;
			if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create an image view");
			}
		}
	}

	void createGraphicsPipeline(void)
	{
		auto vertexShaderCode = readShaderFile("shaders/shader.vert.spv");
		auto fragShaderCode = readShaderFile("shaders/shader.frag.spv");

		VkShaderModule vertexShaderModule = createShaderModule(vertexShaderCode, device);
		VkShaderModule fragShaderModule = createShaderModule(fragShaderCode, device);

		// possible target to move to other file
		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertexShaderModule;
		vertShaderStageInfo.pName = "main";
		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";

		auto bindingDescription = Vertex::getBindingDescription();
		auto attributeDescriptions = Vertex::getAttributeDescriptions();
		VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
		VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo{};
		vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
		vertexInputCreateInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputCreateInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo{};
		inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)swapchainInfo.swapchainExtent.width;
		viewport.height = (float)swapchainInfo.swapchainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.offset = {0, 0};
		scissor.extent = swapchainInfo.swapchainExtent;

		std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
		VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
		dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();

		VkPipelineViewportStateCreateInfo viewportCreateInfo{};
		viewportCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportCreateInfo.viewportCount = 1;
		viewportCreateInfo.scissorCount = 1;

		// rasterizer
		VkPipelineRasterizationStateCreateInfo rasterCreateInfo{};
		rasterCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterCreateInfo.depthClampEnable = VK_FALSE;
		rasterCreateInfo.rasterizerDiscardEnable = VK_FALSE;
		rasterCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
		rasterCreateInfo.lineWidth = 1.0f;
		rasterCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterCreateInfo.depthBiasEnable = VK_FALSE;
		rasterCreateInfo.depthBiasConstantFactor = 0.0f; // optional
		rasterCreateInfo.depthBiasClamp = 0.0f;		 // opt
		rasterCreateInfo.depthBiasSlopeFactor = 0.0f;

		// multisampling
		// basically its turned off for now
		VkPipelineMultisampleStateCreateInfo multisampingCreateInfo{};
		multisampingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampingCreateInfo.sampleShadingEnable = VK_FALSE;
		multisampingCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampingCreateInfo.minSampleShading = 1.0f;
		multisampingCreateInfo.pSampleMask = nullptr;
		multisampingCreateInfo.alphaToCoverageEnable = VK_FALSE;
		multisampingCreateInfo.alphaToOneEnable = VK_FALSE;

		// colorblending
		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

		VkPipelineColorBlendStateCreateInfo colorBlendGlobal{};
		colorBlendGlobal.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendGlobal.logicOpEnable = VK_FALSE;
		colorBlendGlobal.logicOp = VK_LOGIC_OP_COPY;
		colorBlendGlobal.attachmentCount = 1;
		colorBlendGlobal.pAttachments = &colorBlendAttachment;
		colorBlendGlobal.blendConstants[0] = 0.0f;
		colorBlendGlobal.blendConstants[1] = 0.0f;
		colorBlendGlobal.blendConstants[2] = 0.0f;
		colorBlendGlobal.blendConstants[3] = 0.0f;

		// laayout
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo;
		pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCreateInfo.setLayoutCount = 0;
		pipelineLayoutCreateInfo.pSetLayouts = nullptr;
		pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
		pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
		pipelineLayoutCreateInfo.pNext = nullptr; // based on validation layer output
		pipelineLayoutCreateInfo.flags = VK_PIPELINE_LAYOUT_CREATE_INDEPENDENT_SETS_BIT_EXT;

		if (vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("Failed to make pipeline layout");
		}

		// FINALLY
		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputCreateInfo;
		pipelineInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
		pipelineInfo.pViewportState = &viewportCreateInfo;
		pipelineInfo.pRasterizationState = &rasterCreateInfo;
		pipelineInfo.pMultisampleState = &multisampingCreateInfo;
		pipelineInfo.pDepthStencilState = nullptr; // optiona
		pipelineInfo.pColorBlendState = &colorBlendGlobal;
		pipelineInfo.pDynamicState = &dynamicStateCreateInfo;
		pipelineInfo.layout = pipelineLayout;
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex = -1;

		if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
			throw std::runtime_error("failed to create graphics pipeline");
		}

		// wut?
		vkDestroyShaderModule(device, fragShaderModule, nullptr);
		vkDestroyShaderModule(device, vertexShaderModule, nullptr);
	}

	void createRenderPass(void)
	{
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = swapchainInfo.swapchainImageFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create render pass");
		}
	}

	void createFramebuffers(void)
	{
		swapChainFramebuffers.resize(swapChainImageViews.size());
		for (size_t i = 0; i < swapChainImageViews.size(); ++i) {
			VkImageView attachments[] = {swapChainImageViews[i]};
			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = attachments;
			framebufferInfo.width = swapchainInfo.swapchainExtent.width;
			framebufferInfo.height = swapchainInfo.swapchainExtent.height;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("FUCKY WUCKY when make a framebuffer");
			}
		}
	}

	void createCommandPool(void)
	{
		p_device::QueueFamilyIndices queueFamilyIndices = trequirement::findQueuFamilies(physicalDevice, surface);
		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
		if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create command pool");
		}
	}

	void createCommandBuffers(void)
	{
		commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t)MAX_FRAMES_IN_FLIGHT;

		if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate command buffers");
		}
	}

	void recordCommandBuffer(VkCommandBuffer buffer, uint32_t imageIndex)
	{
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0;
		beginInfo.pInheritanceInfo = nullptr;

		if (vkBeginCommandBuffer(buffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error(
			    "are we really going to be throwing exceptions in functions like this?, failed to begin recording command buffer");
		}

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass;
		renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
		renderPassInfo.renderArea.offset = {0, 0};
		renderPassInfo.renderArea.extent = swapchainInfo.swapchainExtent;
		VkClearValue clearColour = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColour;
		vkCmdBeginRenderPass(buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

		// scissor and viewport size are dynamic so set them now
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(swapchainInfo.swapchainExtent.width);
		viewport.height = static_cast<float>(swapchainInfo.swapchainExtent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(buffer, 0, 1, &viewport);
		VkRect2D scissor{};
		scissor.offset = {0, 0};
		scissor.extent = swapchainInfo.swapchainExtent;
		vkCmdSetScissor(buffer, 0, 1, &scissor);

		vkCmdDraw(buffer, 3, 1, 0, 0);
		vkCmdEndRenderPass(buffer);
		if (vkEndCommandBuffer(buffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer");
		}
	}

	void createSyncObjects(void)
	{
		imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

		VkSemaphoreCreateInfo semaphoreInfo{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
		VkFenceCreateInfo fenceInfo{.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {

			auto s1 = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]);
			auto s2 = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]);
			auto s3 = vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]);
			if (s1 != VK_SUCCESS || s2 != VK_SUCCESS || s3 != VK_SUCCESS) {
				throw std::runtime_error("failed to create sync objects");
			}
		}
	}

	void drawFrame(void)
	{
		vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
		uint32_t imageIndex;
		VkResult result =
		    vkAcquireNextImageKHR(device, swapchainInfo.swapchain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			recreateSwapChain();
			return;
		} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
			throw std::runtime_error("Failed to acquire swap chain image");
		}
		vkResetFences(device, 1, &inFlightFences[currentFrame]);
		vkResetCommandBuffer(commandBuffers[currentFrame], 0);
		recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
		VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

		VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit draw command buffer");
		}

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapchains[] = {swapchainInfo.swapchain};
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapchains;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr;

		// omg finally
		result = vkQueuePresentKHR(presentQueue, &presentInfo);
		// subotimal here = we just recreate the swap chain before the next draw
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
			framebufferResized = false;
			recreateSwapChain();
		} else if (result != VK_SUCCESS) {
			throw std::runtime_error("failed to present swap chain image");
		}
		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void recreateSwapChain()
	{
		int width = 0;
		int height = 0;
		glfwGetFramebufferSize(window, &width, &height);
		while (width == 0 || height == 0) {
			// following tutorial but shouldnt the order be reversed?
			glfwGetFramebufferSize(window, &width, &height);
			glfwWaitEvents();
		}
		vkDeviceWaitIdle(device);

		cleanupSwapChain();

		trianglePresentation::createSwapchain(physicalDevice, device, surface, window, swapchainInfo);
		createImageViews();
		createFramebuffers();
	}

	void cleanupSwapChain()
	{
		for (auto fb : swapChainFramebuffers) {
			vkDestroyFramebuffer(device, fb, nullptr);
		}
		for (auto imageView : swapChainImageViews) {
			vkDestroyImageView(device, imageView, nullptr);
		}
		vkDestroySwapchainKHR(device, swapchainInfo.swapchain, nullptr);
	}
};

int main(void)
{
	TriangleApp app;
	try {
		app.run();
	} catch (const std::exception &e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
