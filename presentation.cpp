#include "p_device.hpp"
#include <algorithm>
#include <limits>
#include <optional>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>
//below define and include tells the included header to also include vulkan deps
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "presentation.hpp"
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include "requirement.hpp"

using namespace trianglePresentation;

void trianglePresentation::createSurface(const VkInstance& instance, GLFWwindow *window, VkSurfaceKHR *surface)
{
	if (glfwCreateWindowSurface(instance, window, nullptr, surface) != VK_SUCCESS){
		throw std::runtime_error("Failed to create window surface");
	}
}

VkSurfaceFormatKHR trianglePresentation::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	for (const auto& availableFormat : availableFormats){
		//easy situation
		if (availableFormat.format == VK_FORMAT_B8G8R8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR){
			return availableFormat;
		}
		//'settle' for this first available if fall through
	}
	return availableFormats[0];
}

VkPresentModeKHR trianglePresentation::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availableModes)
{
	//prefer mailbox basically
	if (std::find(availableModes.begin(), availableModes.end(), VK_PRESENT_MODE_MAILBOX_KHR) != availableModes.end()){
		return VK_PRESENT_MODE_MAILBOX_KHR;
	} else {
		return VK_PRESENT_MODE_FIFO_KHR;
	}
}

VkExtent2D trianglePresentation::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window)
{
	//current extent will be correct in many cases, if not it will be set to the max value of uint32
	// it could be incorrect if screen coordinates (used by glfw) differ from pixels or if the window manager gives us leeway to put any value between two bounds
	//in the simple case currentextent will simply be equal to the window size inpixels
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()){
		return capabilities.currentExtent;
	} else {
		//pick w/h
		int width;
		int height;
		glfwGetFramebufferSize(window, &width, &height);
		//im following the tutorial here but wtf if i want to resize my window?
		VkExtent2D actual = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};
		//ensure it is between the min and max capable values
		actual.width = std::clamp(actual.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actual.height = std::clamp(actual.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
		return actual;
	}
}

void trianglePresentation::createSwapchain(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, VkSurfaceKHR surface, GLFWwindow* window, VkSwapchainKHR* handle_swapchain)
{
	auto support = trequirement::querySwapChainSupport(physicalDevice, surface);
	auto format = chooseSwapSurfaceFormat(support.formats);
	auto presentMode = chooseSwapPresentMode(support.presentModes);
	auto extent = chooseSwapExtent(support.capabilities, window);

	uint32_t imageCount = support.capabilities.minImageCount + 1;
	if (support.capabilities.maxImageCount > 0 && imageCount > support.capabilities.maxImageCount)
		imageCount = support.capabilities.maxImageCount;

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = format.format;
	createInfo.imageColorSpace = format.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	p_device::QueueFamilyIndices indices = trequirement::findQueuFamilies(physicalDevice, surface);

	if (indices.graphicsFamily == indices.presentFamily){
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; //better performance
		//rest is optional
	} else {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT; //slower but avoids ownership which i don't learn yet in this chapter
		createInfo.queueFamilyIndexCount = 2;
		uint32_t QueueIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};
		createInfo.pQueueFamilyIndices = QueueIndices;
	}

	createInfo.preTransform = support.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;

	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(logicalDevice, &createInfo, nullptr, handle_swapchain) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create swap chain");
	}
}