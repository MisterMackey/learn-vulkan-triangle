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