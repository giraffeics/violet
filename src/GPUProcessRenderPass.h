#ifndef GPUPROCESSRENDERPASS_H
#define GPUPROCESSRENDERPASS_H

#include "GPUProcess.h"

#include <map>
#include <memory>

#include "GPUEngine.h"
#include "GPUMesh.h"

class GPUProcessRenderPass : public GPUProcess
{
public:
	// constructors and destructor
	GPUProcessRenderPass();
	GPUProcessRenderPass(GPUProcessRenderPass& other) = delete;
	GPUProcessRenderPass(GPUProcessRenderPass&& other) = delete;
	GPUProcessRenderPass& operator=(GPUProcessRenderPass& other) = delete;
	~GPUProcessRenderPass();

	// functions for setting up passable resource relationships
	void setImageViewPR(const PassableResource<VkImageView>* prImageView);
	void setUniformBufferPR(const PassableResource<VkBuffer>* prUniformBuffer);
	void setImageFormatPTR(const VkFormat* format);
	const PassableResource<VkImageView>* getImageViewOutPR();

	// virtual functions inherited from GPUProcess
	virtual std::vector<PRDependency> getPRDependencies();
	virtual VkQueueFlags getNeededQueueType();
	virtual VkCommandBuffer performOperation(VkCommandPool commandPool);
	virtual void acquireLongtermResources();
	virtual void acquireFrameResources();
	virtual void cleanupFrameResources();

private:
	bool createRenderPass();

	const PassableResource<VkImageView>* mPRImageView = nullptr;
	const PassableResource<VkBuffer>* mPRUniformBuffer = nullptr;
	std::unique_ptr<PassableResource<VkImageView>> mPRImageViewOut;
	VkImageView mCurrentImageView = VK_NULL_HANDLE;
	const VkFormat* mImageFormatPTR;
	VkRenderPass mRenderPass;
	GPUPipeline* mPipeline;
	std::map<VkImageView, VkFramebuffer> mFramebuffers;
};

#endif