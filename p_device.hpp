#ifndef TRIANGLE_P_DEVICE_HEADER
#define TRIANGLE_P_DEVICE_HEADER

#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>
//below define and include tells the included header to also include vulkan deps
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace p_device{

void pickPhysicalDevice(VkPhysicalDevice *handle_storage, const VkInstance &instance);

}

#endif