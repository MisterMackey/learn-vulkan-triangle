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

VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availableModes)
{
	//prefer mailbox basically
	if (std::find(availableModes.begin(), availableModes.end(), VK_PRESENT_MODE_MAILBOX_KHR) != availableModes.end()){
		return VK_PRESENT_MODE_MAILBOX_KHR;
	} else {
		return VK_PRESENT_MODE_FIFO_KHR;
	}
}

VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window)
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