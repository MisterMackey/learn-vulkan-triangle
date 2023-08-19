#include "p_device.hpp"
#include <cstdint>
#include <stdexcept>
#include <vulkan/vulkan_core.h>
#include <vector>
#include "p_device.hpp"
#include <set>
#include "requirement.hpp"

using namespace p_device;
using namespace trequirement;

void p_device::pickPhysicalDevice(VkPhysicalDevice *handle_storage, const VkInstance &instance, const VkSurfaceKHR& surface)
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
	if (deviceCount == 0)
		throw std::runtime_error("No Vulkan capable GPU's found");
	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	for (const auto& device: devices) {
		if (isSuitableDevice(device, surface)) {
			*handle_storage = device;
			break;
		}
	}
}

void p_device::createLogicalDevice(VkDevice *handle_device, const VkPhysicalDevice &device, VkQueue *handle_graphicsQueue, VkQueue *handle_presentQueue, const VkSurfaceKHR& surface)
{
	QueueFamilyIndices indices = findQueuFamilies(device, surface);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

	for (uint32_t queueFamily: uniqueQueueFamilies){
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures{}; //features we want, we get back to this later
	//that does it for the queue we want, now to make the device itself
	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
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

	vkGetDeviceQueue(*handle_device, indices.graphicsFamily.value(), 0, handle_graphicsQueue);
	vkGetDeviceQueue(*handle_device, indices.presentFamily.value(), 0, handle_presentQueue);
}
