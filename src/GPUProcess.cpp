#include "GPUProcess.h"

#include "GPUEngine.h"

// GPUProcess default function implementations

/**
 * @brief Tell this GPUProcess what GPUEngine to use.
 * 
 * @param engine 
 */
void GPUProcess::setEngine(GPUEngine* engine)
{
	if (mEngine == nullptr)
		mEngine = engine;
}

/**
 * @brief Return an array of the instance extensions this GPUProcess requires.
 * 
 * @param count Pointer to which the number of instance extensions should be passed.
 * @return const char** 
 */
const char** GPUProcess::getRequiredInstanceExtensions(uint32_t* count)
{
	*count = 0;
	return nullptr;
}

/**
 * @brief Return an array of the device extensions this GPUProcess requires.
 * 
 * @param count Pointer to which the number of device extensions should be passed.
 * @return const char** 
 */
const char** GPUProcess::getRequiredDeviceExtensions(uint32_t* count)
{
	*count = 0;
	return nullptr;
}

/**
 * @brief Return the type of VkQueue that this GPUProcess needs, if this GPUProcess performs a command buffer operation.
 * 
 * @return VkQueueFlags 
 */
VkQueueFlags GPUProcess::getNeededQueueType()
{
	return 0;
}

/**
 * @brief Return the type of operation that this GPUProcess performs.
 * 
 * @return GPUProcess::OperationType 
 */
GPUProcess::OperationType GPUProcess::getOperationType()
{
	return OP_TYPE_COMMAND;
}

/**
 * @brief Perform this GPUProcess's operation by allocating a command buffer, recording commands into it, and returning it.
 * 
 * If getOperationType() does not return OP_TYPE_COMMAND, this function will never be called.
 * After this function is called, the GPUDependencyGraph submits the returned command buffer,
 * and then frees it once it is no longer needed.
 * 
 * @param commandPool The command pool from which to allocate a command buffer.
 * @return VkCommandBuffer 
 */
VkCommandBuffer GPUProcess::performOperation(VkCommandPool commandPool)
{
	return VK_NULL_HANDLE;
}

/**
 * @brief Perform ths GPUProcess's operation, using the given syncronization objects.
 * 
 * If getOperationType() does not return OP_TYPE_OTHER, this function will never be called.
 * 
 * @param waitSemaphores Zero or more semaphores which the operation must wait on.
 * @param fence A fence which the operation must signal.
 * @param semaphore A semaphore which the operation must wait on.
 * @return true Operation was successful.
 * @return false Operation was unsuccessful.
 */
bool GPUProcess::performOperation(std::vector<VkSemaphore> waitSemaphores, VkFence fence, VkSemaphore semaphore)
{
	return true; // do nothing in default implementation
}

/**
 * @brief Return a vector of PRDependency instances with entries for every PassableResource this GPUProcess uses.
 * 
 * The returned vector must not include PRDependencies for any PassableResources which this GPUProcess owns.
 * 
 * @return std::vector<GPUProcess::PRDependency> 
 */
std::vector<GPUProcess::PRDependency> GPUProcess::getPRDependencies()
{
	return std::vector<PRDependency>();
}

/**
 * @brief Acquire all owned resources which will persist for the duration of this GPUProcess's lifetime.
 */
void GPUProcess::acquireLongtermResources()
{
	// do nothing for default implementation
}

/**
 * @brief Acquire all owned resources which may be tied to the GPUEngine's surface.
 */
void GPUProcess::acquireFrameResources()
{
	// do nothing for default implementation
}

/**
 * @brief Free all owned resources which may be tied to the GPUEngine's surface.
 */
void GPUProcess::cleanupFrameResources()
{
	// do nothing for default implementation
}