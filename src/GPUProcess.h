#ifndef GPU_PROCESS_H
#define GPU_PROCESS_H

#include <cstdint>
#include <vector>
#include <vulkan/vulkan.h>

class GPUEngine;

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
		const std::vector<uintptr_t> getPossibleValues() const;

		void setPossibleValues(std::vector<uintptr_t> possibleValues);

	private:
		GPUProcess* mProcess = nullptr;
		uintptr_t* mVkHandle = nullptr;
		std::vector<uintptr_t> mPossibleValues = {};
	};

	typedef struct PRDependency
	{
		const PassableResource* resource;
		VkPipelineStageFlags pipelineStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	};

	// GPUProcess functionality
	void setEngine(GPUEngine* engine);
	virtual const char** getRequiredInstanceExtensions(uint32_t* count);
	virtual const char** getRequiredDeviceExtensions(uint32_t* count);
	virtual VkQueueFlags getNeededQueueType();
	virtual bool isOperationCommand();
	virtual VkCommandBuffer performOperation(VkCommandPool commandPool);
	virtual void performOperation(VkFence fence, VkSemaphore semaphore);
	virtual std::vector<PRDependency> getPRDependencies();
	virtual void acquireLongtermResources();

protected:
	GPUEngine* mEngine = nullptr;
};

class GPUWindowSystem : public GPUProcess
{
public:
	virtual VkSurfaceKHR createSurface(VkInstance instance) = 0;
	virtual VkExtent2D getSurfaceExtent() = 0;
};

#endif
