#ifndef TRIANGLE_REQUIREMENT_H
#define TRIANGLE_REQUIREMENT_H

#include <vector>

#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif

namespace trequirement {

bool checkValidationLayerSupport();
void verifyRequiredExtensionsPresent(const char **required, int nreq);
const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};
std::vector<const char*> getRequiredExtensions();

}

#endif