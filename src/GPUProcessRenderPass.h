#ifndef GPUPROCESSRENDERPASS_H
#define GPUPROCESSRENDERPASS_H

#include "GPUProcess.h"

#include <map>
#include <memory>

#include "GPUEngine.h"

class GPUProcessRenderPass : public GPUProcess
{
public:
	// functions for setting up passable resource relationships
	void setImageViewPR(const PassableResource* prImageView);
	void setImageFormat(VkFormat format);
	const PassableResource* getImageViewOutPR();

	// virtual functions inherited from GPUProcess
	virtual std::vector<PRDependency> getPRDependencies();
	virtual VkQueueFlags getNeededQueueType();
	virtual VkCommandBuffer performOperation(VkCommandPool commandPool);
	virtual void acquireLongtermResources();

private:
	bool createRenderPass();

	const PassableResource* mPRImageView = nullptr;
	std::unique_ptr<PassableResource> mPRImageViewOut;
	VkImageView mCurrentImageView = VK_NULL_HANDLE;
	VkFormat mImageFormat;
	VkRenderPass mRenderPass;
	GPUPipeline* mPipeline;
	std::map<VkImageView, VkFramebuffer> mFramebuffers;
};

#endif