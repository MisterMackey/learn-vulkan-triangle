#ifndef TRIANGLE_PRESENTATION_HEADER
#define TRIANGLE_PRESENTATION_HEADER

#include <optional>
#include <vector>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>
//below define and include tells the included header to also include vulkan deps
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace trianglePresentation{

	void createSurface(const VkInstance& instance, GLFWwindow *window, VkSurfaceKHR *surface);
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availableModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

}

#endif