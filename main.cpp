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
	VkQueue graphicsQueue;

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
		p_device::pickPhysicalDevice(&physicalDevice, vkInstance);
		if (physicalDevice == VK_NULL_HANDLE)
			throw std::runtime_error("failed to find a suitable GPU");
		p_device::createLogicalDevice(&device, physicalDevice, &graphicsQueue);
	}
	void mainLoop(void)
	{
		while (!glfwWindowShouldClose(window)){
			glfwPollEvents();
		}
	}
	void  cleanup(void)
	{
		vkDestroyDevice(device, nullptr);
		if (enableValidationLayers) {
			debugshit::destroyDebugUtilsMesssengerExt(vkInstance, debugMessenger, nullptr);
		}
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
