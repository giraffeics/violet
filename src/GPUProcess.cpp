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

VkCommandBuffer GPUProcess::performOperation(VkCommandPool commandPool)
{
	return VK_NULL_HANDLE;
}

std::vector<GPUProcess::PRDependency> GPUProcess::getPRDependencies()
{
	return std::vector<PRDependency>();
}

void GPUProcess::acquireLongtermResources()
{
	// do nothing for default implementation
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

const std::vector<uintptr_t> GPUProcess::PassableResource::getPossibleValues() const
{
	return mPossibleValues;
}

void GPUProcess::PassableResource::setPossibleValues(std::vector<uintptr_t> possibleValues)
{
	mPossibleValues = possibleValues;
}