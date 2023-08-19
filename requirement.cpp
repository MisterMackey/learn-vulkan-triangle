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

bool checkDeviceExtensionSupport(VkPhysicalDevice device)
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	bool success = true;
	for (const auto& requiredExtension : requiredDeviceExtensions){
		if (std::find_if(availableExtensions.begin(), availableExtensions.end(),
		[&requiredExtension](VkExtensionProperties extension) { return strcmp(requiredExtension, extension.extensionName) == 0; }) != availableExtensions.end()){

			std::cout << "Requirement met: " << requiredExtension << std::endl;
		} else {
			std::cerr << "Requirement not met: " << requiredExtension << std::endl;
			success = false;
		}

	}
	return success;
}

SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
{
	SwapChainSupportDetails details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
	if (formatCount != 0){
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
	}
	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
	if (formatCount != 0) {
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
	}

	return details;
}

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

//not sure if the reference taking is valid here but it seems to work
bool trequirement::isDeviceSuitable(VkPhysicalDevice device, const VkSurfaceKHR& surface)
{
	VkPhysicalDeviceProperties props;
	vkGetPhysicalDeviceProperties(device, &props);
	VkPhysicalDeviceFeatures features;
	vkGetPhysicalDeviceFeatures(device, &features);
	//here would be code that does stuff and decides based on properties and features

	auto queueIndices = findQueuFamilies(device, surface);
	bool extensionsSupported = checkDeviceExtensionSupport(device);
	if (!extensionsSupported) return false;
	auto support = querySwapChainSupport(device, surface);
	bool swapChainAdequate = !support.formats.empty() && !support.presentModes.empty();

	return queueIndices.isComplete() && extensionsSupported && swapChainAdequate;
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