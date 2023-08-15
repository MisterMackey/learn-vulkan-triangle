#ifndef TRIANGLE_P_DEVICE_HEADER
#define TRIANGLE_P_DEVICE_HEADER

#include <optional>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>
//below define and include tells the included header to also include vulkan deps
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace p_device{

const float queuePriority = 1.0f;

void pickPhysicalDevice(VkPhysicalDevice *handle_storage, const VkInstance &instance);
void createLogicalDevice(VkDevice *handle_device, const VkPhysicalDevice& device, VkQueue *handle_graphicsQueue);

struct QueueFamilyIndices{
	std::optional<uint32_t> graphicsFamily;
	bool isComplete()
	{
		//furutre proof
		return graphicsFamily.has_value();
	}
};

QueueFamilyIndices findQueuFamilies(VkPhysicalDevice device);

}

#endif
