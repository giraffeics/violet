#ifndef GPUPROCESSRENDERPASS_H
#define GPUPROCESSRENDERPASS_H

#include "GPUProcess.h"

#include <map>

#include "GPUEngine.h"

class GPUProcessRenderPass : public GPUProcess
{
public:
	// functions for setting passable resource relationships
	void setImageViewPR(const PassableResource* prImageView);

	// virtual functions inherited from GPUProcess
	virtual std::vector<PRDependency> getPRDependencies();
	virtual VkQueueFlags getNeededQueueType();
	virtual VkCommandBuffer performOperation(VkCommandPool commandPool);
	virtual void acquireLongtermResources();

private:
	bool createRenderPass();

	const PassableResource* mPRImageView = nullptr;
	VkRenderPass mRenderPass;
	GPUPipeline* mPipeline;
	std::map<VkImageView, VkFramebuffer> mFramebuffers;
};

#endif