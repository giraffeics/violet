#include "GPUProcessRenderPass.h"

#include "GPUEngine.h"
#include "glm_includes.h"

GPUProcessRenderPass::GPUProcessRenderPass(size_t numSubpasses)
{
	mPRImageViewOut = std::make_unique<PassableResource<VkImageView>>(this, &mCurrentImageView);

	mSubpasses.resize(numSubpasses);
}

GPUProcessRenderPass::~GPUProcessRenderPass()
{
	VkDevice device = mEngine->getDevice();

	cleanupFrameResources();
	vkDestroyRenderPass(device, mRenderPass, nullptr);
}

/**
 * @brief Get a pointer to the Subpass specified by a given index.
 * 
 * This must be used to access each Subpass so that its details, including
 * the shader and attachments, can be specified.
 * 
 * @param index 
 * @return GPUProcessRenderPass::Subpass* 
 */
GPUProcessRenderPass::Subpass* GPUProcessRenderPass::getSubpass(size_t index)
{
	return &(((Subpass*)(mSubpasses.data()))[index]);
}

/**
 * @brief Assign a PassableImageView for this GPURenderPass to render color information to.
 * 
 * All VkImageViews passed through prImageView must be able to be used
 * as a color attachment, and their resolution must match all other
 * attachments passed to this GPURenderPass.
 * 
 * @param prImageView 
 */
void GPUProcessRenderPass::setImageViewPR(const PassableImageView* prImageView)
{
	mPRImageView = prImageView;
}

/**
 * @brief Assign a PassableImageView for this GPURenderPass to render depth information to.
 * 
 * All VkImageViews passed through prZBufferView must be able to be used
 * as a depth attachment, and their resolution must match all other
 * attachments passed to this GPURenderPass.
 * 
 * @param prZBufferView 
 */
void GPUProcessRenderPass::setZBufferViewPR(const PassableImageView* prZBufferView)
{
	mPRZBufferView = prZBufferView;
}

/**
 * @brief Assign a PassableResource<VkBuffer> for this GPURenderPass to read from as a uniform buffer.
 * 
 * It is currently assumed that the passed prUniformBuffer belongs to the GPUEngine's GPUMeshWrangler.
 * This relationship will probably be changed later so that the GPUProcessRenderPass can automatically
 * find the GPUMeshWrangler without another class having to call setUniformBufferPR.
 * 
 * @param prUniformBuffer 
 */
void GPUProcessRenderPass::setUniformBufferPR(const PassableResource<VkBuffer>* prUniformBuffer)
{
	mPRUniformBuffer = prUniformBuffer;
}

/**
 * @brief Get a const pointer to a PassableResource<VkImageView> that can be passed to another process.
 * 
 * This must be used to syncronize presentation of the color attachment after rendering.
 * When the PassableResource returned by getImageViewOutPR() is passed to a GPUProcessPresent,
 * this GPUProcessRenderPass's color attachment must be obtained from the corresponding GPUProcessSwapchain.
 * This function will later be useful to facilitate render-to-texture, although other functionality
 * must first be implemented to support that.
 * 
 * @return const GPUProcess::PassableResource<VkImageView>* 
 */
const GPUProcess::PassableResource<VkImageView>* GPUProcessRenderPass::getImageViewOutPR()
{
	return mPRImageViewOut.get();
}

std::vector<GPUProcess::PRDependency>  GPUProcessRenderPass::getPRDependencies()
{
	return std::vector<PRDependency>({ 
		{mPRImageView, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
		{mPRUniformBuffer, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT},
		{mPRZBufferView, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT}
	});
}

VkQueueFlags GPUProcessRenderPass::getNeededQueueType()
{
	return VK_QUEUE_GRAPHICS_BIT;
}

VkCommandBuffer GPUProcessRenderPass::performOperation(VkCommandPool commandPool)
{
	// Acquire passed-in resources
	mCurrentImageView = (VkImageView)mPRImageView->getVkHandle();

	// Record command buffer
	VkCommandBuffer commandBuffer = mEngine->allocateCommandBuffer(commandPool);

	// begin recording
	{
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.pNext = nullptr;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		beginInfo.pInheritanceInfo = nullptr;
		vkBeginCommandBuffer(commandBuffer, &beginInfo);
	}

	// begin and end render pass
	{
		std::vector<GPUMesh::AttributeType> attributeTypes = {GPUMesh::MESH_ATTRIBUTE_POSITION, GPUMesh::MESH_ATTRIBUTE_NORMAL};

		glm::vec3 tvec = { 0.0f, 0.0f, -3.0f };
		auto extent = mEngine->getSurfaceExtent();
		glm::mat4 viewProjection = glm::perspective(45.0f, ((float)extent.width / (float)extent.height), 0.01f, 100.0f) * glm::translate(glm::identity<glm::mat4>(), tvec);

		VkClearValue clearValues[2];
		clearValues[0].color = { 0.8f, 0.1f, 0.3f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };

		VkRenderPassBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		beginInfo.pNext = nullptr;
		beginInfo.renderPass = mRenderPass;
		beginInfo.framebuffer = mFramebuffers.find(mCurrentImageView)->second;
		beginInfo.renderArea.offset = { 0, 0 };
		beginInfo.renderArea.extent = mEngine->getSurfaceExtent();
		beginInfo.clearValueCount = 2;
		beginInfo.pClearValues = clearValues;
		vkCmdBeginRenderPass(commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

		// iterate through all subpasses
		mSubpasses[0].draw(commandBuffer, mEngine, &viewProjection);
		for(size_t i=1; i<mSubpasses.size(); i++)
		{
			vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
			mSubpasses[i].draw(commandBuffer, mEngine, &viewProjection);
		}

		vkCmdEndRenderPass(commandBuffer);
	}

	vkEndCommandBuffer(commandBuffer);

	return commandBuffer;
}

void GPUProcessRenderPass::acquireLongtermResources()
{
	// create RenderPass
	createRenderPass();

	// acquire long-term subpass resources
	for(uint32_t i=0; i<mSubpasses.size(); i++)
		mSubpasses[i].acquireLongtermResources(mRenderPass, i, mEngine);
}

void GPUProcessRenderPass::acquireFrameResources()
{
	for(auto& subpass : mSubpasses)
		subpass.acquireFrameResources();

	// create framebuffers for each possible ImageView
	auto possibleImageViews = mPRImageView->getPossibleValues();
	for (auto imageView : possibleImageViews)
	{
		VkFramebuffer framebuffer;
		VkExtent2D extent = mPRImageView->getExtent();
		VkImageView attachments[2] = {
			imageView,
			mPRZBufferView->getPossibleValues()[0]
		};

		VkFramebufferCreateInfo framebufferCreateInfo = {};
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.pNext = nullptr;
		framebufferCreateInfo.flags = 0;
		framebufferCreateInfo.renderPass = mRenderPass;
		framebufferCreateInfo.attachmentCount = 2;
		framebufferCreateInfo.pAttachments = attachments;
		framebufferCreateInfo.width = extent.width;
		framebufferCreateInfo.height = extent.height;
		framebufferCreateInfo.layers = 1;

		vkCreateFramebuffer(mEngine->getDevice(), &framebufferCreateInfo, nullptr, &framebuffer);
		mFramebuffers.insert({ imageView, framebuffer });
	}

	// set up resources that are passed out
	mPRImageViewOut->setPossibleValues(mPRImageView->getPossibleValues());
}

void GPUProcessRenderPass::cleanupFrameResources()
{
	VkDevice device = mEngine->getDevice();

	for (auto& framebuffer : mFramebuffers)
	{
		vkDestroyFramebuffer(device, framebuffer.second, nullptr);
	}

	mFramebuffers.clear();

	for(auto& subpass : mSubpasses)
		subpass.cleanupFrameResources();
}

/**
 * @brief Checks for a shared attachment between two attachment arrays.
 * 
 * If one is found, updates the subpass dependency accordingly.
 */
inline void checkForSharedAttachment(VkSubpassDependency& dependency, uint32_t earlyAttachmentCount, const VkAttachmentReference* pEarlyAttachments,
								uint32_t lateAttachmentAcount, const VkAttachmentReference* pLateAttachments,
								VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, VkAccessFlags srcAccess, VkAccessFlags dstAccess)
{
	for(size_t srcIndex = 0; srcIndex < earlyAttachmentCount; srcIndex++)
	{
		for(size_t dstIndex = 0; dstIndex < lateAttachmentAcount; dstIndex++)
		{
			if(pEarlyAttachments[srcIndex].attachment == pLateAttachments[dstIndex].attachment)
			{
				dependency.srcAccessMask |= srcAccess;
				dependency.dstAccessMask |= dstAccess;
				dependency.srcStageMask |= srcStage;
				dependency.dstStageMask |= dstStage;
				return;
			}
		}
	}
}

/**
 * @brief Generates a vector containing the necessary subpass dependencies for all subpasses.
 * 
 * @return std::vector<VkSubpassDependency> 
 */
std::vector<VkSubpassDependency> GPUProcessRenderPass::generateSubpassDependencies()
{
	// obtain subpass descriptions
	std::vector<VkSubpassDescription> subpassDescriptions(mSubpasses.size());
	for(size_t i=0; i<mSubpasses.size(); i++)
		subpassDescriptions[i] = mSubpasses[i].getDescription();

	// generate dependencies
	std::vector<VkSubpassDependency> dependencies;
	for(size_t laterIndex=1; laterIndex<mSubpasses.size(); laterIndex++)
	{
		for(size_t earlierIndex = 0; earlierIndex < laterIndex; earlierIndex++)
		{
			VkSubpassDependency dependency{};
			dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
			dependency.srcSubpass = earlierIndex;
			dependency.dstSubpass = laterIndex;
			auto& srcDescription = subpassDescriptions[earlierIndex];
			auto& dstDescription = subpassDescriptions[laterIndex];
			
			checkForSharedAttachment(dependency, srcDescription.colorAttachmentCount, srcDescription.pColorAttachments,
										dstDescription.colorAttachmentCount, dstDescription.pColorAttachments,
										VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
										VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

			checkForSharedAttachment(dependency, srcDescription.colorAttachmentCount, srcDescription.pColorAttachments,
										dstDescription.inputAttachmentCount, dstDescription.pInputAttachments,
										VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
										VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT);

			checkForSharedAttachment(dependency, srcDescription.inputAttachmentCount, srcDescription.pInputAttachments,
										dstDescription.colorAttachmentCount, dstDescription.pColorAttachments,
										VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
										VK_ACCESS_COLOR_ATTACHMENT_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

			checkForSharedAttachment(dependency, srcDescription.inputAttachmentCount, srcDescription.pInputAttachments,
										dstDescription.inputAttachmentCount, dstDescription.pInputAttachments,
										VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
										VK_ACCESS_COLOR_ATTACHMENT_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT);

			if(dependency.srcStageMask != 0)
				dependencies.push_back(dependency);
		}
	}

	return dependencies;
}



bool GPUProcessRenderPass::createRenderPass()
{
	// create render pass
	std::vector<Attachment> attachments(2);
	Attachment& colorAttachment = attachments[0];
	colorAttachment.setPRImageViewIn(mPRImageView);
	colorAttachment.setLoadOp(VK_ATTACHMENT_LOAD_OP_CLEAR);
	colorAttachment.setStoreOp(VK_ATTACHMENT_STORE_OP_STORE);
	colorAttachment.setFinalLayout(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	Attachment& depthAttachcment = attachments[1];
	depthAttachcment.setPRImageViewIn(mPRZBufferView);
	depthAttachcment.setLoadOp(VK_ATTACHMENT_LOAD_OP_CLEAR);
	depthAttachcment.setFinalLayout(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	std::vector<VkAttachmentDescription> attachmentDescriptions(attachments.size());
	for(size_t i=0; i<attachments.size(); i++)
		attachmentDescriptions[i] = attachments[i].getDescription();

	// obtain subpass descriptions
	std::vector<VkSubpassDescription> subpassDescriptions(mSubpasses.size());
	for(size_t i=0; i<mSubpasses.size(); i++)
		subpassDescriptions[i] = mSubpasses[i].getDescription();

	// obtain subpass dependencies
	auto subpassDependencies = generateSubpassDependencies();


	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.pNext = nullptr;
	renderPassCreateInfo.attachmentCount = attachmentDescriptions.size();
	renderPassCreateInfo.pAttachments = attachmentDescriptions.data();
	renderPassCreateInfo.subpassCount = mSubpasses.size();
	renderPassCreateInfo.pSubpasses = subpassDescriptions.data();
	renderPassCreateInfo.dependencyCount = subpassDependencies.size();
	renderPassCreateInfo.pDependencies = subpassDependencies.data();

	if (vkCreateRenderPass(mEngine->getDevice(), &renderPassCreateInfo, nullptr, &mRenderPass) != VK_SUCCESS)
		return false;

	return true;
}

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
// GPUProcessRenderPass::Subpass implementation
//
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

void GPUProcessRenderPass::Subpass::setShader(std::string name, VkShaderStageFlags stageFlags)
{
	mShaderName = name;
	mShaderStageFlags = stageFlags;
}

void GPUProcessRenderPass::Subpass::setInputAttachments(std::vector<VkAttachmentReference>&& attachmentReferences)
{
	mInputAttachments = std::move(attachmentReferences);
}

void GPUProcessRenderPass::Subpass::setColorAttachments(std::vector<VkAttachmentReference>&& attachmentReferences)
{
	mColorAttachments = std::move(attachmentReferences);
}

void GPUProcessRenderPass::Subpass::setDepthAttachment(VkAttachmentReference attachmentReference)
{
	mDepthAttachment = attachmentReference;
}

void GPUProcessRenderPass::Subpass::preserve(uint32_t attachment)
{
	// TODO
}

void GPUProcessRenderPass::Subpass::setAttributeTypes(std::vector<GPUMesh::AttributeType>&& attributeTypes)
{
	mAttributeTypes = attributeTypes;
}

/**
 * @brief Acquires longterm resources for this subpass.
 * 
 * This includes shader loading and compilation, and creation of the pipeline.
 * 
 * @param renderPass 
 */
void GPUProcessRenderPass::Subpass::acquireLongtermResources(VkRenderPass renderPass, uint32_t subpass, GPUEngine* engine)
{
	std::vector<std::string> shaderFileNames;
	std::vector<VkShaderStageFlagBits> shaderStages;

	if(mShaderStageFlags & VK_SHADER_STAGE_VERTEX_BIT)
	{
		shaderFileNames.push_back(mShaderName + "_vert");
		shaderStages.push_back(VK_SHADER_STAGE_VERTEX_BIT);
	}
	if(mShaderStageFlags & VK_SHADER_STAGE_FRAGMENT_BIT)
	{
		shaderFileNames.push_back(mShaderName + "_frag");
		shaderStages.push_back(VK_SHADER_STAGE_FRAGMENT_BIT);
	}

	mPipeline = std::make_unique<GPUPipeline>(engine, shaderFileNames, shaderStages, renderPass, subpass, mAttributeTypes);
}

void GPUProcessRenderPass::Subpass::acquireFrameResources()
{
	mPipeline->validate();
}

void GPUProcessRenderPass::Subpass::cleanupFrameResources()
{
	mPipeline->invalidate();
}

VkSubpassDescription GPUProcessRenderPass::Subpass::getDescription()
{
	return {
		0,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		(uint32_t) mInputAttachments.size(),
		mInputAttachments.data(),
		(uint32_t) mColorAttachments.size(),
		mColorAttachments.data(),
		nullptr,
		&mDepthAttachment,
		(uint32_t) mPreserveAttachments.size(),
		mPreserveAttachments.data()
	};
}

void GPUProcessRenderPass::Subpass::draw(VkCommandBuffer commandBuffer, GPUEngine* engine, glm::mat4* viewProjection)
{
	VkPipelineLayout pipelineLayout = mPipeline->getLayout();
	GPUMeshWrangler* meshWrangler = engine->getMeshWrangler();

	mPipeline->bind(commandBuffer);
	vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), viewProjection);
	for (auto instance : meshWrangler->getMeshInstances())
	{
		meshWrangler->bindModelDescriptor(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, instance);
		instance->mMesh->draw(commandBuffer, mAttributeTypes);
	}
}

VkAttachmentDescription GPUProcessRenderPass::Attachment::getDescription()
{
	return {
		0,
		mPRImageViewIn->getFormat(),
		VK_SAMPLE_COUNT_1_BIT,
		mLoadOp,
		mStoreOp,
		mStencilLoadOp,
		mStencilStoreOp,
		VK_IMAGE_LAYOUT_UNDEFINED,
		mFinalLayout
	};
}
