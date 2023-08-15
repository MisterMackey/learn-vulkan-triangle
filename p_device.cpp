#include "p_device.hpp"
#include <stdexcept>
#include <vulkan/vulkan_core.h>
#include <vector>
#include "p_device.hpp"

using namespace p_device;

bool isSuitable(VkPhysicalDevice device)
{
	VkPhysicalDeviceProperties props;
	vkGetPhysicalDeviceProperties(device, &props);
	VkPhysicalDeviceFeatures features;
	vkGetPhysicalDeviceFeatures(device, &features);
	//here would be code that does stuff and decides based on properties and features

	auto queueIndices = findQueuFamilies(device);
	return queueIndices.isComplete();
}

void p_device::pickPhysicalDevice(VkPhysicalDevice *handle_storage, const VkInstance &instance)
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
	if (deviceCount == 0)
		throw std::runtime_error("No Vulkan capable GPU's found");
	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	for (const auto& device: devices) {
		if (isSuitable(device)) {
			*handle_storage = device;
			break;
		}
	}
}

void p_device::createLogicalDevice(VkDevice *handle_device, const VkPhysicalDevice &device)
{
	QueueFamilyIndices indices = findQueuFamilies(device);

	VkDeviceQueueCreateInfo queueCreateInfo{};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
	queueCreateInfo.queueCount = 1;
	queueCreateInfo.pQueuePriorities = &queuePriority;
	VkPhysicalDeviceFeatures deviceFeatures{}; //features we want, we get back to this later

	//that does it for the queue we want, now to make the device itself
	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = 1;
	createInfo.pQueueCreateInfos = &queueCreateInfo;
	createInfo.pEnabledFeatures = &deviceFeatures;
	//for older implementations, the info for validation layers should be set. newer implementations ignore this
	//because there is no more distinction between device and instance specific validation layers.
	//to be compatible this info should be set, but im lazy and the info is in another file so im leaving it blank for now
	//to be clear, there are still NON VALIDATION layers specifc to devices and those would be set here
	createInfo.enabledLayerCount = 0;
	createInfo.ppEnabledLayerNames = nullptr;

	if (vkCreateDevice(device, &createInfo, nullptr, handle_device) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create logical vulkan device.");
	}
}

p_device::QueueFamilyIndices p_device::findQueuFamilies(VkPhysicalDevice device)
{
	QueueFamilyIndices indices;

	//retrieve list of queu families supported by device
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	//were gonna look for a queue family with the vk queue graphics bit support
	//if i did this myself i would probably just use std iterator tools to find the ref and pass that around but im following a tutorial so...
	int i = 0;
	for (const auto& queueFamily : queueFamilies) {
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily = i;
			if (indices.isComplete()) 
				break;
		}
		i++;
	}

	return indices;
}