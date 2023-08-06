#ifndef TRIANGLE_DEBUGSHIT_H
#define TRIANGLE_DEBUGSHIT_H

#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>
//below define and include tells the included header to also include vulkan deps
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>


namespace debugshit {

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallBack(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT *callbackData,
	void *userData);


VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
					const VkAllocationCallbacks *allocator, VkDebugUtilsMessengerEXT *debugMessenger);

void destroyDebugUtilsMesssengerExt(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* allocator);
void populateDebugMessengerStruct(VkDebugUtilsMessengerCreateInfoEXT &createInfo);

}

#endif