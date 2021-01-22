#include "GPUProcessCompletionAlert.h"

#include <future>
#include <iostream>

#include "GPUEngine.h"

// constructors and destructor
GPUProcessCompletionAlert::~GPUProcessCompletionAlert()
{
    VkDevice device = mEngine->getDevice();
    if(mFence != VK_NULL_HANDLE)
        vkDestroyFence(device, mFence, nullptr);
}

// public functionality
void GPUProcessCompletionAlert::setPR(const PassableResourceBase* pr)
{
    mPassableResource = pr;
}

// virtual functions inherited from GPUProcess
void GPUProcessCompletionAlert::acquireLongtermResources()
{
    mFence = mEngine->createFence(0);
    mEngine->createBuffer(  32, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mBuffer, mBufferMemory);
}

std::vector<GPUProcess::PRDependency> GPUProcessCompletionAlert::getPRDependencies()
{
    return {{mPassableResource}};
}

bool GPUProcessCompletionAlert::performOperation(std::vector<VkSemaphore> waitSemaphores, VkFence fence, VkSemaphore semaphore)
{
    vkResetFences(mEngine->getDevice(), 1, &mFence);

    VkCommandBuffer commandBuffer = mEngine->allocateCommandBuffer(mEngine->getGraphicsPool());

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.pNext = nullptr;
    beginInfo.pInheritanceInfo = nullptr;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    vkCmdFillBuffer(commandBuffer, mBuffer, 0, 4, 0);   // dummy work so the GPU has something to wait for
    vkEndCommandBuffer(commandBuffer);

    std::vector<VkPipelineStageFlags> flags(waitSemaphores.size());
    for(size_t i=0; i<flags.size(); i++)
        flags[i] = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = VK_NULL_HANDLE;
    submitInfo.waitSemaphoreCount = waitSemaphores.size();
    submitInfo.pWaitSemaphores = waitSemaphores.data();
    submitInfo.pWaitDstStageMask = flags.data();
    submitInfo.signalSemaphoreCount = (semaphore == VK_NULL_HANDLE) ? 0 : 1;
    submitInfo.pSignalSemaphores = &semaphore;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    vkQueueSubmit(mEngine->getGraphicsQueue(), 1, &submitInfo, mFence);

    std::async(std::launch::async, [this]{
        vkWaitForFences(mEngine->getDevice(), 1, &mFence, VK_TRUE, UINT64_MAX);
        std::cout << "PROCESS COMPLETED!!" << std::endl;
    });
}