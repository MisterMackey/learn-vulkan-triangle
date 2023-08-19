#ifndef TRIANGLE_REQUIREMENT_H
#define TRIANGLE_REQUIREMENT_H

#include <vector>
#include <vulkan/vulkan_core.h>
#include "p_device.hpp"

#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif

const std::vector<const char*> requiredDeviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

namespace trequirement {

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

bool checkValidationLayerSupport();
void verifyRequiredExtensionsPresent(const char **required, int nreq);
const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};
std::vector<const char*> getRequiredExtensions();
bool isDeviceSuitable(VkPhysicalDevice device, const VkSurfaceKHR& surface);
p_device::QueueFamilyIndices findQueuFamilies(VkPhysicalDevice device, const VkSurfaceKHR& surface);

}

#endif