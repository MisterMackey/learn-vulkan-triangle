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