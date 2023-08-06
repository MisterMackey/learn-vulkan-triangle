#include "p_device.hpp"
#include <stdexcept>
#include <vulkan/vulkan_core.h>
#include <vector>

bool isSuitable(VkPhysicalDevice device)
{
	return true;
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
