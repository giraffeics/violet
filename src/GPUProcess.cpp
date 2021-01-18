#include "GPUProcess.h"

#include "GPUEngine.h"

// GPUProcess default function implementations

void GPUProcess::setEngine(GPUEngine* engine)
{
	if (mEngine == nullptr)
		mEngine = engine;
}

const char** GPUProcess::getRequiredInstanceExtensions(uint32_t* count)
{
	*count = 0;
	return nullptr;
}

const char** GPUProcess::getRequiredDeviceExtensions(uint32_t* count)
{
	*count = 0;
	return nullptr;
}

VkQueueFlags GPUProcess::getNeededQueueType()
{
	return 0;
}

bool GPUProcess::isOperationCommand()
{
	return true;
}

VkCommandBuffer GPUProcess::performOperation(VkCommandPool commandPool)
{
	return VK_NULL_HANDLE;
}

void GPUProcess::performOperation(std::vector<VkSemaphore> waitSemaphores, VkFence fence, VkSemaphore semaphore)
{
	// do nothing in default implementation
}

std::vector<GPUProcess::PRDependency> GPUProcess::getPRDependencies()
{
	return std::vector<PRDependency>();
}

void GPUProcess::acquireLongtermResources()
{
	// do nothing for default implementation
}

void GPUProcess::acquireFrameResources()
{
	// do nothing for default implementation
}

void GPUProcess::cleanupFrameResources()
{
	// do nothing for default implementation
}