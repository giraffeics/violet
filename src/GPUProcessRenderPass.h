#ifndef GPUPROCESSRENDERPASS_H
#define GPUPROCESSRENDERPASS_H

#include "GPUProcess.h"

#include <map>
#include <memory>

#include "GPUEngine.h"

class GPUProcessRenderPass : public GPUProcess
{
public:
	GPUProcessRenderPass();

	// functions for setting up passable resource relationships
	void setImageViewPR(const PassableResource<VkImageView>* prImageView);
	void setImageFormatPTR(const VkFormat* format);
	const PassableResource<VkImageView>* getImageViewOutPR();

	// virtual functions inherited from GPUProcess
	virtual std::vector<PRDependency> getPRDependencies();
	virtual VkQueueFlags getNeededQueueType();
	virtual VkCommandBuffer performOperation(VkCommandPool commandPool);
	virtual void acquireLongtermResources();

private:
	bool createRenderPass();

	const PassableResource<VkImageView>* mPRImageView = nullptr;
	std::unique_ptr<PassableResource<VkImageView>> mPRImageViewOut;
	VkImageView mCurrentImageView = VK_NULL_HANDLE;
	const VkFormat* mImageFormatPTR;
	VkRenderPass mRenderPass;
	GPUPipeline* mPipeline;
	std::map<VkImageView, VkFramebuffer> mFramebuffers;
};

#endif