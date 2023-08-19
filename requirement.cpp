#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <string.h>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <vulkan/vulkan_core.h>
//below define and include tells the included header to also include vulkan deps
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "requirement.hpp"
#include <iostream>

using namespace trequirement;

bool trequirement::checkValidationLayerSupport(void)
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	auto returnSuccess = true;

	for (const auto requiredLayer : validationLayers) {
		if (std::none_of(availableLayers.begin(), availableLayers.end(), [&requiredLayer](VkLayerProperties layer) {
			return (strcmp(requiredLayer, layer.layerName) == 0);
		})) {
			std::cerr << "Validation layer not found! " << requiredLayer << std::endl;
			returnSuccess = false;
		} else {
			std::cout << "Validation layer found " << requiredLayer << std::endl;
		}
	}

	return returnSuccess;
}

void trequirement::verifyRequiredExtensionsPresent(const char **required, int nreq)
{
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

	bool success = true;
	for (int i =0; i < nreq; ++i) {
		const char *requirement = required[i];
		if (std::none_of(extensions.begin(), extensions.end(),
			[&requirement](VkExtensionProperties vkextension) { return (strcmp(requirement, vkextension.extensionName) == 0); })){
			success = false;
			std::cerr << "Requirement not met! " << requirement << std::endl;
		} else {
			std::cout << "Requirement met: " << requirement << std::endl;
		}
	}
	if (!success)
		throw std::runtime_error("One ore more requirements are missing\n");
}

std::vector<const char*> trequirement::getRequiredExtensions()
{
	uint32_t extCnt = 0;
	const char **extensions;
	extensions = glfwGetRequiredInstanceExtensions(&extCnt);
	std::vector<const char*> extvec(extensions, extensions + extCnt);
	if (enableValidationLayers){
		extvec.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}
	return extvec;
}

bool trequirement::isSuitableDevice(VkPhysicalDevice device, const VkSurfaceKHR& surface)
{
	VkPhysicalDeviceProperties props;
	vkGetPhysicalDeviceProperties(device, &props);
	VkPhysicalDeviceFeatures features;
	vkGetPhysicalDeviceFeatures(device, &features);
	//here would be code that does stuff and decides based on properties and features

	auto queueIndices = findQueuFamilies(device, surface);
	return queueIndices.isComplete();
}

p_device::QueueFamilyIndices trequirement::findQueuFamilies(VkPhysicalDevice device, const VkSurfaceKHR& surface)
{
	p_device::QueueFamilyIndices indices;

	//retrieve list of queu families supported by device
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	//were gonna look for a queue family with the vk queue graphics bit support
	//if i did this myself i would probably just use std iterator tools to find the ref and pass that around but im following a tutorial so...
	int i = 0;
	for (const auto& queueFamily : queueFamilies) {
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily = i;
			if (indices.isComplete())
				break;
		}
		VkBool32 presentationSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentationSupport);
		if (presentationSupport){
			indices.presentFamily = i;
			if (indices.isComplete())
				break;
		}
		i++;
	}

	return indices;
}