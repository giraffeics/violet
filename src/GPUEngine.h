#ifndef GPUENGINE_H
#define GPUENGINE_H

#include <string>
#include <vector>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include "GPUFeatureSet.h"

class GPUEngine
{
public:
	GPUEngine(const std::vector<GPUFeatureSet*>& featureSets, std::string appName, std::string engineName, uint32_t appVersion = 0, uint32_t engineVersion = 0);

	// Public Getters
	VkInstance getInstance() { return mInstance; }
	VkDevice getDevice() { return mLogicalDevice; }

private:
	bool createInstance(const std::vector<const char*> extensions, std::string appName, std::string engineName, uint32_t appVersion, uint32_t engineVersion);
	bool choosePhysicalDevice(const std::vector<const char*> extensions);
	std::vector<const char*> createExtensionsVector(const std::vector<GPUFeatureSet*>& featureSets);

	VkInstance mInstance = VK_NULL_HANDLE;

	VkDevice mLogicalDevice = VK_NULL_HANDLE;
	VkPhysicalDevice mPhysicalDevice = VK_NULL_HANDLE;
};

#endif