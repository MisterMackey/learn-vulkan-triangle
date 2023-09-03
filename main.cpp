#include <cstddef>
#include <cstdint>
#include <exception>
#include <sys/types.h>
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
#include <array>
#include <cstdlib>
#include <glm/glm.hpp>
#include <iostream>
#include <stdexcept>
#include <string.h>
#include <vector>
#define GLM_FORCE_RADIANS
#include "stb_linking.hpp"
#include <chrono>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

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

struct UniformBufferObject {
	// be explicit abt alignments, it needs to match the vulkan spec once it goes to the shader
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

// clang-format off
const std::vector<Vertex> vertices = {
	{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
	{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
	{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
	{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}},
};

const std::vector<uint16_t> indices = {
	0, 1, 2, 2, 3, 0
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
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;
	VkCommandPool commandPool;
	VkCommandPool memoryTransferCommandPool;
	uint32_t currentFrame = 0;
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;
	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;
	std::vector<void *> uniformBuffersMapped;
	std::vector<VkCommandBuffer> commandBuffers;
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	VkImage textureImage;
	VkDeviceMemory textureImageMemory;

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
		createDescriptorSetLayout();
		createGraphicsPipeline();
		createFramebuffers();
		createCommandPools();
		createTextureImage();
		createVertexBuffers();
		createIndexBuffers();
		createUniformBuffers();
		createDescriptorPool();
		createDescriptorSets();
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
		vkDestroyCommandPool(device, memoryTransferCommandPool, nullptr);
		cleanupSwapChain();
		vkDestroyBuffer(device, indexBuffer, nullptr);
		vkFreeMemory(device, indexBufferMemory, nullptr);
		vkDestroyBuffer(device, vertexBuffer, nullptr);
		vkFreeMemory(device, vertexBufferMemory, nullptr);
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			vkDestroyBuffer(device, uniformBuffers[i], nullptr);
			vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
		}
		vkDestroyDescriptorPool(device, descriptorPool, nullptr);
		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
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
		rasterCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
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
		pipelineLayoutCreateInfo.setLayoutCount = 1;
		pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
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

	void createCommandPools(void)
	{
		p_device::QueueFamilyIndices queueFamilyIndices = trequirement::findQueuFamilies(physicalDevice, surface);
		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
		if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create command pool");
		}

		VkCommandPoolCreateInfo memPoolInfo{};
		memPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		memPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
		poolInfo.queueFamilyIndex =
		    queueFamilyIndices.graphicsFamily
			.value(); // this queue supports mem transfer implicitly, its possible to use a seperate queue JUST for mem transfers
		if (vkCreateCommandPool(device, &memPoolInfo, nullptr, &memoryTransferCommandPool) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create memory transfer command pool");
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
		VkBuffer vertexBuffers[] = {vertexBuffer};
		VkDeviceSize offsets[] = {0};
		vkCmdBindVertexBuffers(buffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(buffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);

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

		// pre-index
		// vkCmdDraw(buffer, static_cast<uint32_t>(vertices.size()), 1, 0, 0);
		vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);
		vkCmdDrawIndexed(buffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
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

		updateUniformBuffer(currentFrame);

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

	void createVertexBuffers(void)
	{
		VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			     stagingBuffer, stagingBufferMemory);

		void *data;
		vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, vertices.data(), (size_t)bufferSize);
		vkUnmapMemory(device, stagingBufferMemory);

		createBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			     vertexBuffer, vertexBufferMemory);

		copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
	}

	void createIndexBuffers(void)
	{
		VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			     stagingBuffer, stagingBufferMemory);

		void *data;
		vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, indices.data(), (size_t)bufferSize);
		vkUnmapMemory(device, stagingBufferMemory);

		createBuffer(bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer,
			     indexBufferMemory);

		copyBuffer(stagingBuffer, indexBuffer, bufferSize);

		vkDestroyBuffer(device, stagingBuffer, nullptr);
		vkFreeMemory(device, stagingBufferMemory, nullptr);
	}

	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
	{
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = memoryTransferCommandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

		// recorrd that shit
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = 0;
		copyRegion.dstOffset = 0;
		copyRegion.size = size;

		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(graphicsQueue);

		vkFreeCommandBuffers(device, memoryTransferCommandPool, 1, &commandBuffer);
	}

	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
	{
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
		for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}
		throw std::runtime_error("No suitable VRAM found");
	}

	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &buffer, VkDeviceMemory &bufferMemory)
	{
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create vertex buffer");
		}

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

		if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
			throw std::runtime_error("Failed to allocate vertex buffer memory");
		}
		vkBindBufferMemory(device, buffer, bufferMemory, 0);
	}

	void createDescriptorSetLayout(void)
	{
		VkDescriptorSetLayoutBinding uboLayoutBinding{};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		uboLayoutBinding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = 1;
		layoutInfo.pBindings = &uboLayoutBinding;

		if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create descriptor set layout");
		}
	}

	void createUniformBuffers(void)
	{
		VkDeviceSize bufferSize = sizeof(UniformBufferObject);
		uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
		uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				     uniformBuffers[i], uniformBuffersMemory[i]);

			vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
		}
	}

	void updateUniformBuffer(uint32_t currentImage)
	{
		static auto startTime = std::chrono::high_resolution_clock::now();
		auto currentTime = std::chrono::high_resolution_clock::now();

		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

		UniformBufferObject ubo{};
		ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.proj =
		    glm::perspective(glm::radians(45.0f), swapchainInfo.swapchainExtent.width / (float)swapchainInfo.swapchainExtent.height, 0.1f, 10.0f);

		/*
		GLM was originally designed for OpenGL, where the Y coordinate of the clip coordinates is inverted. The easiest way to compensate for that is to
		flip the sign on the scaling factor of the Y axis in the projection matrix. If you don't do this, then the image will be rendered upside down.
		*/
		ubo.proj[1][1] *= -1;

		/*
		Using a UBO this way is not the most efficient way to pass frequently changing values to the shader. A more efficient way to pass a small buffer
		of data to shaders are push constants. We may look at these in a future chapter.*/
		memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
	}

	void createDescriptorPool(void)
	{
		VkDescriptorPoolSize poolSize{};
		poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = 1;
		poolInfo.pPoolSizes = &poolSize;
		poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		poolInfo.flags = 0; // optional, can be set to indicate that the sets can be freed during runtime

		if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create descriptor pool");
		}
	}

	void createDescriptorSets(void)
	{
		std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		allocInfo.pSetLayouts = layouts.data();

		descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
		if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate descriptor sets!");
		}
		// will be automatically freeds when the pool is destroyed

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = uniformBuffers[i];
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject);

			VkWriteDescriptorSet descriptorWrite{};
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = descriptorSets[i];
			descriptorWrite.dstBinding = 0;
			descriptorWrite.dstArrayElement = 0;
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrite.descriptorCount = 1;
			descriptorWrite.pBufferInfo = &bufferInfo;
			descriptorWrite.pImageInfo = nullptr;
			descriptorWrite.pTexelBufferView = nullptr;

			vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);
		}
	}

	void createTextureImage(void)
	{
		int texWidth;
		int texHeight;
		int texChannels;
		stbi_uc *pixels = stbi_load("textures/texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		VkDeviceSize imageSize = texWidth * texHeight * 4;

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			     stagingBuffer, stagingBufferMemory);

		void *data;
		vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
		memcpy(data, pixels, static_cast<size_t>(imageSize));
		vkUnmapMemory(device, stagingBufferMemory);
		stbi_image_free(pixels);

		createImage(texWidth, texHeight, VK_FORMAT_R8G8B8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);
	}

	void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
			 VkImage &image, VkDeviceMemory &imageMemory)
	{
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = format;
		imageInfo.tiling = tiling;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = usage;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.flags = 0;

		if (vkCreateImage(device, &imageInfo, nullptr, &textureImage) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create vk image for texture");
		}

		VkMemoryRequirements memRequirements;
		vkGetImageMemoryRequirements(device, textureImage, &memRequirements);
		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		if (vkAllocateMemory(device, &allocInfo, nullptr, &textureImageMemory) != VK_SUCCESS) {
			throw std::runtime_error("Failed to allocate memory for image");
		}

		vkBindImageMemory(device, textureImage, textureImageMemory, 0);
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
