#ifndef GPU_PROCESS_H
#define GPU_PROCESS_H

#include <cstdint>
#include <vector>
#include <vulkan/vulkan.h>

class GPUEngine;

class GPUProcess
{
public:
	enum OperationType
	{
		OP_TYPE_COMMAND,
		OP_TYPE_OTHER,
		OP_TYPE_NOOP
	};

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

	class PassableImageView : public PassableResource<VkImageView>
	{
	public:
		PassableImageView(GPUProcess* process, VkImageView* handle) 
			: PassableResource<VkImageView>(process, handle) {}

		void setExtent(VkExtent2D& extent){
			mExtent = extent;
		}

		void setFormat(VkFormat format) {
			mFormat = format;
		}

		VkExtent2D getExtent() const {
			return mExtent;
		}

		VkFormat getFormat() const {
			return mFormat;
		}

	private:
		VkExtent2D mExtent{};
		VkFormat mFormat;
	};

	struct PRDependency
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
	virtual OperationType getOperationType();
	virtual VkCommandBuffer performOperation(VkCommandPool commandPool);
	virtual bool performOperation(std::vector<VkSemaphore> waitSemaphores, VkFence fence, VkSemaphore semaphore);
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
