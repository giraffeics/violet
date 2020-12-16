#include "GPUProcess.h"

// GPUProcess default function implementations

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
VkCommandBuffer GPUProcess::performOperation(VkCommandPool commandPool)
{
	return VK_NULL_HANDLE;
}
std::vector<GPUProcess::PRDependency> GPUProcess::getPRDependencies()
{
	return std::vector<GPUProcess::PRDependency>();
}

// PassableResource function implementations

GPUProcess::PassableResource::PassableResource(GPUProcess* process, uintptr_t* handle)
{
	mProcess = process;
	mVkHandle = handle;
}

GPUProcess* GPUProcess::PassableResource::getSourceProcess() const
{
	return mProcess;
}

uintptr_t GPUProcess::PassableResource::getVkHandle() const
{
	return *mVkHandle;
}