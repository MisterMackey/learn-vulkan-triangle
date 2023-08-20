#ifndef SHADER_LOADING_HEADER
#define SHADER_LOADING_HEADER
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_core.h>

std::vector<char> readShaderFile(const std::string& filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		std::string err = std::string( "Failed to open file: ").append(filename);
		throw std::runtime_error(err);
	}

	size_t fileSize = (size_t) file.tellg();
	std::vector<char> buffer(fileSize);
	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();

	return buffer;
}

VkShaderModule createShaderModule(const std::vector<char>& code, VkDevice logicalDevice)
{
	VkShaderModuleCreateInfo createInfo {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
	VkShaderModule module;
	if (vkCreateShaderModule(logicalDevice, &createInfo, nullptr, &module) != VK_SUCCESS) {
		throw std::runtime_error("Failed to initialize shader module");
	}
	return module;
}

#endif
