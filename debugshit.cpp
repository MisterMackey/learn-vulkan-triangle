#include "debugshit.hpp"
#include <iostream>
#include <vulkan/vulkan_core.h>

static void populateDebugMessengerStruct(VkDebugUtilsMessengerCreateInfoEXT &createInfo)
{
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugshit::debugCallBack;
	createInfo.pUserData = nullptr;
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugshit::debugCallBack(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT *callbackData,
	void *userData)
{
	
	std::cerr << "validation layer: " << callbackData->pMessage << std::endl;
	return VK_FALSE;
}

VkResult debugshit::CreateDebugUtilsMessengerEXT(VkInstance instance, 
					const VkAllocationCallbacks *allocator, VkDebugUtilsMessengerEXT *debugMessenger)
{
	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	populateDebugMessengerStruct(createInfo);
	auto lookedUpFunc = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (lookedUpFunc != nullptr)
		return lookedUpFunc(instance, &createInfo, allocator, debugMessenger);
	else
		return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void debugshit::destroyDebugUtilsMesssengerExt(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* allocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
		func(instance, debugMessenger, allocator);
}
