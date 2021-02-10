#ifndef GPUPROCESSRENDERPASS_H
#define GPUPROCESSRENDERPASS_H

#include "GPUProcess.h"

#include <map>
#include <memory>

#include "GPUEngine.h"
#include "GPUPipeline.h"
#include "GPUMesh.h"

/**
 * @brief A GPUProcess which performs a render pass.
 * 
 * Currently renders all staged mesh isntances from the GPUEngine's GPUMeshWrangler.
 * Functionality needs to be expanded a lot to make more complex rendering possible.
 */
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
	void setImageViewPR(const PassableImageView* prImageView);
	void setUniformBufferPR(const PassableResource<VkBuffer>* prUniformBuffer);
	void setZBufferViewPR(const PassableImageView* prZBufferView);
	const PassableResource<VkImageView>* getImageViewOutPR();

	// virtual functions inherited from GPUProcess
	virtual std::vector<PRDependency> getPRDependencies();
	virtual VkQueueFlags getNeededQueueType();
	virtual VkCommandBuffer performOperation(VkCommandPool commandPool);
	virtual void acquireLongtermResources();
	virtual void acquireFrameResources();
	virtual void cleanupFrameResources();

private:
	class Subpass
	{
	public:
		void setInputAttachments(std::vector<VkAttachmentReference>&& attachmentReferences);
		void setColorAttachments(std::vector<VkAttachmentReference>&& attachmentReferences);
		void setDepthAttachment(VkAttachmentReference attachmentReference);
		void preserve(uint32_t attachment);
		VkSubpassDescription getDescription();

	private:
		std::vector<VkAttachmentReference> mInputAttachments;
		std::vector<VkAttachmentReference> mColorAttachments;
		VkAttachmentReference mDepthAttachment;
		std::vector<uint32_t> mPreserveAttachments;
	};

	bool createRenderPass();

	const PassableImageView* mPRImageView = nullptr;
	const PassableImageView* mPRZBufferView = nullptr;
	const PassableResource<VkBuffer>* mPRUniformBuffer = nullptr;
	std::unique_ptr<PassableResource<VkImageView>> mPRImageViewOut;
	VkImageView mCurrentImageView = VK_NULL_HANDLE;
	VkRenderPass mRenderPass;
	GPUPipeline* mPipeline;
	std::map<VkImageView, VkFramebuffer> mFramebuffers;
};

#endif