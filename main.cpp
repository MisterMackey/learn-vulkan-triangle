#include <cstddef>
#include <cstdint>
#include <exception>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>
//below define and include tells the included header to also include vulkan deps
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <string.h>
#include "requirement.hpp"
#include "p_device.hpp"
#include "debugshit.hpp"
#include "presentation.hpp"
#include "shaderLoading.hpp"

const int windowHeight = 800;
const int windowWidth = 600;

class TriangleApp{
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
	VkDevice device; //logical device
	VkDebugUtilsMessengerEXT debugMessenger;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkSurfaceKHR surface;
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	trianglePresentation::swapchainInformation swapchainInfo;
	std::vector<VkImageView> swapChainImageViews;

	void initWindow(void)
	{
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		window = glfwCreateWindow(windowHeight, windowWidth, "Vulkanerino", nullptr, nullptr);
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
		//im giving up on splitting all this shit up into files. I don't know enough to properly factor this shit anyway.
		//so im just following the tutorial now
		createGraphicsPipeline();

	}
	void mainLoop(void)
	{
		while (!glfwWindowShouldClose(window)){
			glfwPollEvents();
		}
	}
	void  cleanup(void)
	{
		for (auto imageView : swapChainImageViews){
			vkDestroyImageView(device, imageView, nullptr);
		}
		vkDestroySwapchainKHR(device, swapchainInfo.swapchain, nullptr);
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
		if (enableValidationLayers && !trequirement::checkValidationLayerSupport()){
			throw std::runtime_error("validation layer fucky wucky");
		}
		VkApplicationInfo appInfo {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Hello Triangle";
		appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo createInfo {};
		//basic info
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		auto required_extensions = trequirement::getRequiredExtensions();

		trequirement::verifyRequiredExtensionsPresent(required_extensions.data(), static_cast<uint32_t>(required_extensions.size()));

		createInfo.enabledExtensionCount = static_cast<uint32_t>(required_extensions.size());
		createInfo.ppEnabledExtensionNames = required_extensions.data();

		//which global validation layers to enable
		//this should sit in trequirement now

		//this variable is defined outside the if block to make sure its not destroyed before the vkcreateinstance call
		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		if (enableValidationLayers){
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
		for (size_t i =0; i< swapchainInfo.swapchainImages.size(); ++i) {
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
			if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS){
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

		//possible target to move to other file
		VkPipelineShaderStageCreateInfo vertShaderStageInfo {};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertexShaderModule;
		vertShaderStageInfo.pName = "main";
		VkPipelineShaderStageCreateInfo fragShaderStageInfo {};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

		//wut?
		vkDestroyShaderModule(device, fragShaderModule, nullptr);
		vkDestroyShaderModule(device, vertexShaderModule, nullptr);
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
