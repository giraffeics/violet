#ifndef GPUENGINE_H
#define GPUENGINE_H

#include <string>
#include <vector>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include "GPUProcess.h"

class GPUPipeline;

class GPUEngine
{
public:
	// Constructor
	GPUEngine(const std::vector<GPUProcess*>& processes, GPUWindowSystem* windowSystem, std::string appName, std::string engineName, uint32_t appVersion = 0, uint32_t engineVersion = 0);

	// Public functionality
	void renderFrame();
	VkCommandBuffer allocateCommandBuffer(VkCommandPool commandPool);

	// Public Getters
	VkInstance getInstance() { return mInstance; }
	VkDevice getDevice() { return mDevice; }
	VkExtent2D getSurfaceExtent() { return mSurfaceExtent; }
	VkRenderPass getRenderPass() { return mRenderPass; }
	VkFormat getSurfaceFormat() { return mSurfaceFormat.format; }

private:
	struct Frame
	{
		VkImageView mImageView;
		VkImage mImage;
	};
	std::vector<Frame> mFrames;

	bool createInstance(const std::vector<const char*>& extensions, std::string appName, std::string engineName, uint32_t appVersion, uint32_t engineVersion);
	bool choosePhysicalDevice(const std::vector<const char*>& extensions);
	bool createLogicalDevice(const std::vector<const char*>& extensions);
	bool createSurface();
	bool createFrames();
	bool chooseSurfaceFormat();
	bool createCommandPools();
	bool createSwapchain();
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

	GPUWindowSystem* mWindowSystem;

	static constexpr uint32_t INVALID_QUEUE_FAMILY = std::numeric_limits<uint32_t>::max();

	static std::vector<const char*> validationLayers;

	// Temporary objects for testing GPUProcess system
	GPUProcess::PassableResource* mPRImageView;
	GPUProcess* mRenderPassProcess;
	VkImageView currentImageView;

	// Vulkan objects owned by GPUEngine
	VkInstance mInstance = VK_NULL_HANDLE;
	uint32_t mGraphicsQueueFamily = INVALID_QUEUE_FAMILY;
	uint32_t mPresentQueueFamily = INVALID_QUEUE_FAMILY;
	VkQueue mGraphicsQueue = VK_NULL_HANDLE;
	VkQueue mPresentQueue = VK_NULL_HANDLE;
	VkCommandPool mGraphicsCommandPool = VK_NULL_HANDLE;
	VkDevice mDevice = VK_NULL_HANDLE;
	VkPhysicalDevice mPhysicalDevice = VK_NULL_HANDLE;
	VkSurfaceKHR mSurface = VK_NULL_HANDLE;
	VkExtent2D mSurfaceExtent;
	VkSurfaceFormatKHR mSurfaceFormat;
	VkSwapchainKHR mSwapchain = VK_NULL_HANDLE;
	VkRenderPass mRenderPass = VK_NULL_HANDLE;
	VkFence mFence = VK_NULL_HANDLE;
	GPUPipeline* mPipeline = nullptr;
};

class GPUPipeline
{
public:
	GPUPipeline(GPUEngine* engine, std::vector<std::string> shaderNames, std::vector<VkShaderStageFlagBits> shaderStages, VkRenderPass renderPass);
	bool valid();
	void bind(VkCommandBuffer commandBuffer);
private:
	GPUEngine* mEngine;
	std::vector<VkShaderModule> mShaderModules;
	VkPipelineLayout mPipelineLayout;
	VkPipeline mPipeline;
};

#endif