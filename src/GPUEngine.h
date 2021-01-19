#ifndef GPUENGINE_H
#define GPUENGINE_H

#include <string>
#include <vector>
#include <memory>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include "GPUProcess.h"
#include "GPUDependencyGraph.h"
#include "GPUMeshWrangler.h"

class GPUEngine
{
public:
	// Constructors & Destructor
	GPUEngine(const std::vector<GPUProcess*>& processes, GPUWindowSystem* windowSystem, std::string appName, std::string engineName, uint32_t appVersion = 0, uint32_t engineVersion = 0);
	GPUEngine(GPUEngine& other) = delete;
	GPUEngine(GPUEngine&& other) = delete;
	GPUEngine& operator=(GPUEngine& other) = delete;
	~GPUEngine();

	// Public functionality
	void renderFrame();
	VkCommandBuffer allocateCommandBuffer(VkCommandPool commandPool);
	VkSemaphore createSemaphore();
	VkFence createFence(VkFenceCreateFlags flags);
	bool createBuffer(VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryFlags, VkBuffer& buffer, VkDeviceMemory& memory);
	void transferToBuffer(VkBuffer destination, void* data, VkDeviceSize size, VkDeviceSize offset);
	uint32_t findMemoryType(uint32_t memoryTypeBits, VkMemoryPropertyFlags properties);

	// Public Getters
	VkInstance getInstance() { return mInstance; }
	VkDevice getDevice() { return mDevice; }
	VkPhysicalDevice getPhysicalDevice() { return mPhysicalDevice; }
	uint32_t getGraphicsQueueFamily() { return mGraphicsQueueFamily; }
	uint32_t getPresentQueueFamily() { return mPresentQueueFamily; }
	VkQueue getPresentQueue() { return mPresentQueue; }
	VkCommandPool getGraphicsPool() { return mGraphicsCommandPool; }
	VkQueue getGraphicsQueue() { return mGraphicsQueue; }
	VkSurfaceKHR getSurface() { return mSurface; }
	VkExtent2D getSurfaceExtent() { return mSurfaceExtent; }
	VkRenderPass getRenderPass() { return mRenderPass; }
	VkDescriptorSetLayout getModelDescriptorLayout() { return mDescriptorLayoutModel; }
	GPUMeshWrangler* getMeshWrangler() { return mMeshWrangler.get(); }

private:
	bool createInstance(const std::vector<const char*>& extensions, std::string appName, std::string engineName, uint32_t appVersion, uint32_t engineVersion);
	bool choosePhysicalDevice(const std::vector<const char*>& extensions);
	bool createLogicalDevice(const std::vector<const char*>& extensions);
	bool createSurface();
	bool createCommandPools();
	bool createDescriptorSetLayout();
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

	// GPUProcess objects;
	// TODO: move these handles elsewhere, as the engine should
	// not be responsible for deciding what GPUProcesses are used
	GPUProcess* mZBufferImage;
	GPUProcess* mRenderPassProcess;
	VkImageView currentImageView;
	GPUProcess* mSwapchainProcess;
	GPUProcess* mPresentProcess;
	std::unique_ptr<GPUDependencyGraph> mDependencyGraph;
	std::unique_ptr<GPUMeshWrangler> mMeshWrangler;

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
	VkRenderPass mRenderPass = VK_NULL_HANDLE;
	VkFence mTransferFence = VK_NULL_HANDLE;
	VkDescriptorSetLayout mDescriptorLayoutModel = VK_NULL_HANDLE;
};

#endif