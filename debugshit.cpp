#include "debugshit.hpp"
#include <iostream>
#include <vulkan/vulkan_core.h>

VKAPI_ATTR VkBool32 VKAPI_CALL debugshit::debugCallBack(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT *callbackData,
	void *userData)
{
	
	std::cerr << "validation layer: " << callbackData->pMessage << std::endl;
	return VK_FALSE;
}

VkResult debugshit::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *createInfo,
					const VkAllocationCallbacks *allocator, VkDebugUtilsMessengerEXT *debugMessenger)
{
	auto lookedUpFunc = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (lookedUpFunc != nullptr)
		return lookedUpFunc(instance, createInfo, allocator, debugMessenger);
	else
		return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void debugshit::destroyDebugUtilsMesssengerExt(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* allocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
		func(instance, debugMessenger, allocator);
}