#ifndef GPUENGINE_H
#define GPUENGINE_H

#include <string>
#include <vector>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include "GPUProcess.h"

class GPUEngine
{
public:
	GPUEngine(const std::vector<GPUProcess*>& processes, GPUWindowSystem* windowSystem, std::string appName, std::string engineName, uint32_t appVersion = 0, uint32_t engineVersion = 0);

	// Public Getters
	VkInstance getInstance() { return mInstance; }
	VkDevice getDevice() { return mLogicalDevice; }

private:
	bool createInstance(const std::vector<const char*>& extensions, std::string appName, std::string engineName, uint32_t appVersion, uint32_t engineVersion);
	bool choosePhysicalDevice(const std::vector<const char*>& extensions);
	bool createLogicalDevice(const std::vector<const char*>& extensions);
	bool createCommandPools();
	static std::vector<const char*> createInstanceExtensionsVector(const std::vector<GPUProcess*>& processes);
	static std::vector<const char*> createDeviceExtensionsVector(const std::vector<GPUProcess*>& processes);
	static std::vector<uint32_t> findDeviceQueueFamilies(VkPhysicalDevice device, std::vector<VkQueueFlags>& flags);
	static uint32_t findDevicePresentQueueFamily(VkPhysicalDevice device, VkSurfaceKHR surface);

	template<typename T> static inline void fillExtensionsInStruct(T& structure, const std::vector<const char*>& extensions)
	{
		uint32_t numExtensions = extensions.size();
		structure.enabledExtensionCount = numExtensions;
		structure.ppEnabledExtensionNames = (numExtensions == 0) ? nullptr : extensions.data();
	}

	static constexpr uint32_t INVALID_QUEUE_FAMILY = std::numeric_limits<uint32_t>::max();

	VkInstance mInstance = VK_NULL_HANDLE;

	uint32_t mGraphicsQueueFamily = INVALID_QUEUE_FAMILY;
	VkQueue mGraphicsQueue = VK_NULL_HANDLE;
	VkCommandPool mGraphicsCommandPool = VK_NULL_HANDLE;
	VkDevice mLogicalDevice = VK_NULL_HANDLE;
	VkPhysicalDevice mPhysicalDevice = VK_NULL_HANDLE;
	VkSurfaceKHR mSurface = VK_NULL_HANDLE;
};

#endif