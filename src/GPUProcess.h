#ifndef GPU_PROCESS_H
#define GPU_PROCESS_H

#include <cstdint>
#include <vector>
#include <vulkan/vulkan.h>

class GPUEngine;

/**
 * @brief Base class for all processes which utilize the GPU and can be managed by a GPUDependencyGraph.
 * 
 * A GPUProcess can perform an operation by generating a command buffer, doing something
 * else and signaling the syncronization object(s) passed to it, or can have no operation.
 * To facilitate this, there are two function signatures for performOperation(), and a
 * child class can implement one or neither of them; also, a subclass can implement the
 * getOperationType() function to specify an operation type other than generating a
 * command buffer.
 * 
 * GPUProcesses can pass resources between each other during the execution of the dependency
 * graph, using the PassableResource and PRDependency nested classes. A GPUProcess which owns such
 * a resource should also own a PassableResource for that resource. Said PassableResource
 * is to be updated with possible values upon resource acquisition or re-acquisition,
 * and updated with the current value when performOperation() is called. If this GPUProcess
 * has no operation, the current value is to be set upon resource acquisition or re-acquisition.
 * Owned PassableResource instances should generally be made available to other classes as
 * a const pointer, through a public getter function.
 * 
 * If a GPUProcess uses any PassableResources which are owned by another GPUProcess, it
 * must override getPRDependencies() so that it can describe what PassableResources it uses,
 * and at which pipeline stage(s). The GPUDependencyGraph will analyze these relationships to
 * determine the proper execution order and potentially group command buffer submits together.
 * 
 * GPUProcess is not meant to be instantiated; however, there is no single function which
 * all child classes must override, so I have not found a good way to make this class
 * pure virtual without adding a lot of clutter to all child class implementation files.
 */
class GPUProcess
{
public:
	enum OperationType
	{
		OP_TYPE_COMMAND,
		OP_TYPE_OTHER,
		OP_TYPE_NOOP
	};

	/**
	 * @brief Base class for PassableResource.
	 * 
	 * Provides minimal functionality so that PRDependency instances can be made and used
	 * without PRDependency needing to be a generic type.
	 */
	class PassableResourceBase
	{
	public:
		GPUProcess* getSourceProcess() const {
			return mProcess;
		}

	protected:
		// do not allow instances of base class
		~PassableResourceBase() {};

		GPUProcess* mProcess = nullptr;
	};
	
	/**
	 * @brief Generic PassableResource class allowing various resource types to be passed safely.
	 * 
	 * Intended for the passing of Vulkan resources which have a handle and do not have any associated
	 * metadata which must also be passed. If there is associated metadata that must be passed along
	 * with the Vulkan handle, a subclass such as PassableImageView should be used. If there is
	 * no suitable subclass, a new one may need to be created.
	 * 
	 * @tparam T Type of resource handle that will be passed.
	 */
	template<typename T> class PassableResource : public PassableResourceBase
	{
	public:
		/**
		 * @brief Construct a new PassableResource object.
		 * 
		 * @param process The GPUProcess responsible for managing the passed handle.
		 * @param handle Pointer to a variable where the passed handle resides.
		 */
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

	/**
	 * @brief A PassableResource subclass for passing VkImageViews and associated metadata.
	 * 
	 * Currently, the extent and format of the image view can be passed through this class.
	 * The GPUProcess which owns a PassableImageView instance is expected to set the format
	 * and extent whenever they change. The format and extent should only change when resources
	 * are acquired or re-acquired.
	 */
	class PassableImageView : public PassableResource<VkImageView>
	{
	public:
		/**
		 * @brief Construct a new PassableImageView object.
		 * 
		 * @param process The GPUProcess responsible for managing the passed handle and setting associated metadata.
		 * @param handle Pointer to a variable where the passed handle resides.
		 */
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

	/**
	 * @brief Structure describing a dependency on a passable resource.
	 * 
	 * Contains a const pointer to the passable resource, as well as a set of flags describing
	 * which pipeline stage(s) the resource is used in.
	 */
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

/**
 * @brief A virtual subtype of GPUProcess which is used to interface with a windowing API.
 * 
 * This functionality may later be made to not use a GPUProcess, or may be merged with GPUProcessSwapchain.
 */
class GPUWindowSystem : public GPUProcess
{
public:
	virtual VkSurfaceKHR createSurface(VkInstance instance) = 0;
	virtual VkExtent2D getSurfaceExtent() = 0;
};

#endif
