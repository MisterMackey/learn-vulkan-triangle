#include <algorithm>
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