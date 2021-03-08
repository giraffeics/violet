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
	class Attachment
	{
	public:
		void setClearColor(const VkClearColorValue& color){ mClearValue.color = color; }
		void setClearDepthStencil(const VkClearDepthStencilValue& depthStencil){ mClearValue.depthStencil = depthStencil; }
		void setLoadOp(VkAttachmentLoadOp loadOp){ mLoadOp = loadOp; }
		void setStoreOp(VkAttachmentStoreOp storeOp){ mStoreOp = storeOp; }
		void setStencilLoadOp(VkAttachmentLoadOp loadOp){ mStencilLoadOp = loadOp; }
		void setStencilStoreOp(VkAttachmentStoreOp storeOp){ mStencilStoreOp = storeOp; }
		void setFinalLayout(VkImageLayout finalLayout){ mFinalLayout = finalLayout; }
		void setPRImageViewIn(const PassableImageView* prImageViewIn){ mPRImageViewIn = prImageViewIn; }

		VkClearValue getClearValue(){ return mClearValue; }
		VkAttachmentDescription getDescription();

	private:
		VkAttachmentLoadOp mLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		VkAttachmentStoreOp mStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		VkAttachmentLoadOp mStencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		VkAttachmentStoreOp mStencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		VkImageLayout mFinalLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		VkClearValue mClearValue;
		const PassableImageView* mPRImageViewIn = nullptr;
	};

	class Subpass
	{
	public:
		void setShader(std::string name, VkShaderStageFlags stageFlags);
		void setInputAttachments(std::vector<VkAttachmentReference>&& attachmentReferences);
		void setColorAttachments(std::vector<VkAttachmentReference>&& attachmentReferences);
		void setDepthAttachment(VkAttachmentReference attachmentReference);
		void preserve(uint32_t attachment);
		void setAttributeTypes(std::vector<GPUMesh::AttributeType>&& attributeTypes);

		void acquireLongtermResources(VkRenderPass renderPass, uint32_t subpass, GPUEngine* engine);
		void acquireFrameResources();
		void cleanupFrameResources();
		VkSubpassDescription getDescription();

		void draw(VkCommandBuffer commandBuffer, GPUEngine* engine, glm::mat4* viewProjection);

	private:
		std::vector<VkAttachmentReference> mInputAttachments;
		std::vector<VkAttachmentReference> mColorAttachments;
		VkAttachmentReference mDepthAttachment;
		std::vector<uint32_t> mPreserveAttachments;
		std::vector<GPUMesh::AttributeType> mAttributeTypes;

		std::string mShaderName;
		VkShaderStageFlags mShaderStageFlags;
		std::unique_ptr<GPUPipeline> mPipeline;
	};

	// constructors and destructor
	GPUProcessRenderPass(size_t numSubpasses);
	GPUProcessRenderPass(GPUProcessRenderPass& other) = delete;
	GPUProcessRenderPass(GPUProcessRenderPass&& other) = delete;
	GPUProcessRenderPass& operator=(GPUProcessRenderPass& other) = delete;
	~GPUProcessRenderPass();

	// functions for setting up subpasses
	Subpass* getSubpass(size_t index);

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
	// private helper functions
	std::vector<VkSubpassDependency> generateSubpassDependencies();
	bool createRenderPass();

	// private member variables
	const PassableImageView* mPRImageView = nullptr;
	const PassableImageView* mPRZBufferView = nullptr;
	const PassableResource<VkBuffer>* mPRUniformBuffer = nullptr;
	std::unique_ptr<PassableResource<VkImageView>> mPRImageViewOut;
	VkImageView mCurrentImageView = VK_NULL_HANDLE;
	VkRenderPass mRenderPass;
	std::vector<Subpass> mSubpasses;
	std::map<VkImageView, VkFramebuffer> mFramebuffers;
};

#endif