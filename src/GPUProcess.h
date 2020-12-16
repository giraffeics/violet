#ifndef GPU_PROCESS_H
#define GPU_PROCESS_H

#include <cstdint>
#include <vector>
#include <vulkan/vulkan.h>

class GPUProcess
{
public:
	// classes & structs scoped to GPUProcess
	class PassableResource
	{
	public:
		PassableResource(GPUProcess* process, uintptr_t* handle);

		uintptr_t getVkHandle() const;
		GPUProcess* getSourceProcess() const;

	private:
		GPUProcess* mProcess = nullptr;
		uintptr_t* mVkHandle = nullptr;
	};

	struct PRDependency
	{
		PassableResource* resource;
		VkPipelineStageFlags pipelineStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	};

	// GPUProcess functionality
	virtual const char** getRequiredInstanceExtensions(uint32_t* count);
	virtual const char** getRequiredDeviceExtensions(uint32_t* count);
	virtual VkQueueFlags getNeededQueueType();
	virtual VkCommandBuffer performOperation(VkCommandPool commandPool);
	virtual std::vector<GPUProcess::PRDependency> getPRDependencies();
};

class GPUWindowSystem : public GPUProcess
{
public:
	virtual VkSurfaceKHR createSurface(VkInstance instance) = 0;
	virtual VkExtent2D getSurfaceExtent() = 0;
};

#endif
