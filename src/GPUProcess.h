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
	class PassableResourceBase
	{
	public:
		GPUProcess* getSourceProcess() const {
			return mProcess;
		}

	protected:
		// do not allow instances of base class
		~PassableResourceBase() {};

	protected:
		GPUProcess* mProcess = nullptr;
	};
	
	template<typename T> class PassableResource : public PassableResourceBase
	{
	public:
		PassableResource(GPUProcess* process, T* handle){
			mProcess = process;
			mVkHandle = handle;
		}

		T getVkHandle() const {
			return *mVkHandle;
		}
		const std::vector<T> getPossibleValues() const {
			return mPossibleValues;
		}

		void setPossibleValues(std::vector<T> possibleValues) {
			mPossibleValues = possibleValues;
		}

	private:
		T* mVkHandle = nullptr;
		std::vector<T> mPossibleValues = {};
	};

	typedef struct PRDependency
	{
		const PassableResourceBase* resource;
		VkPipelineStageFlags pipelineStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	};

	// GPUProcess functionality
	virtual ~GPUProcess() = default;
	void setEngine(GPUEngine* engine);
	virtual const char** getRequiredInstanceExtensions(uint32_t* count);
	virtual const char** getRequiredDeviceExtensions(uint32_t* count);
	virtual VkQueueFlags getNeededQueueType();
	virtual bool isOperationCommand();
	virtual VkCommandBuffer performOperation(VkCommandPool commandPool);
	virtual void performOperation(std::vector<VkSemaphore> waitSemaphores, VkFence fence, VkSemaphore semaphore);
	virtual std::vector<PRDependency> getPRDependencies();
	virtual void acquireLongtermResources();
	virtual void acquireFrameResources();
	virtual void cleanupFrameResources();

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
