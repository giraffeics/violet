#ifndef GPU_PROCESS_H
#define GPU_PROCESS_H

#include <cstdint>
#include <vulkan/vulkan.h>

class GPUProcess
{
public:
	virtual const char** getRequiredInstanceExtensions(uint32_t* count)
	{
		*count = 0;
		return nullptr;
	}
	virtual const char** getRequiredDeviceExtensions(uint32_t* count)
	{
		*count = 0;
		return nullptr;
	}
	virtual VkQueueFlags getNeededQueueType()
	{
		return 0;
	}
	virtual VkCommandBuffer performOperation(VkCommandPool commandPool)
	{
		return VK_NULL_HANDLE;
	}

protected:
	struct DependencyEdge
	{
		GPUProcess* dependency = nullptr;
		GPUProcess* dependee = nullptr;
		bool isDeviceLocal = true;
		VkPipelineStageFlags waitStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	};
};

class GPUWindowSystem : public GPUProcess
{
public:
	virtual VkSurfaceKHR createSurface(VkInstance instance) = 0;
	virtual VkExtent2D getSurfaceExtent() = 0;
};

#endif