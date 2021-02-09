#include "GPUProcessRenderPass.h"

#include "GPUEngine.h"
#include "glm_includes.h"

GPUProcessRenderPass::GPUProcessRenderPass()
{
	mPRImageViewOut = std::make_unique<PassableResource<VkImageView>>(this, &mCurrentImageView);
}

GPUProcessRenderPass::~GPUProcessRenderPass()
{
	VkDevice device = mEngine->getDevice();

	cleanupFrameResources();
	vkDestroyRenderPass(device, mRenderPass, nullptr);
	delete mPipeline;
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
		std::vector<GPUMesh::AttributeType> attributeTypes = {GPUMesh::MESH_ATTRIBUTE_POSITION};

		VkPipelineLayout pipelineLayout = mPipeline->getLayout();
		GPUMeshWrangler* meshWrangler = mEngine->getMeshWrangler();
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

		mPipeline->bind(commandBuffer);
		vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &viewProjection);
		for (auto instance : meshWrangler->getMeshInstances())
		{
			meshWrangler->bindModelDescriptor(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, instance);
			instance->mMesh->draw(commandBuffer, attributeTypes);
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

	// create pipeline
	mPipeline = new GPUPipeline(mEngine, 
		{ "passthrough_vert", "passthrough_frag" }, 
		{ VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT },
		mRenderPass);
}

void GPUProcessRenderPass::acquireFrameResources()
{
	mPipeline->validate();

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

	mPipeline->invalidate();
}

bool GPUProcessRenderPass::createRenderPass()
{
	// create render pass
	VkAttachmentDescription attachmentDescriptions[2];
	attachmentDescriptions[0].flags = 0;
	attachmentDescriptions[0].format = mPRImageView->getFormat();
	attachmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	attachmentDescriptions[1].flags = 0;
	attachmentDescriptions[1].format = mPRZBufferView->getFormat();
	attachmentDescriptions[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDescriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDescriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescriptions[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescriptions[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescriptions[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDescriptions[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentReference = {};
	colorAttachmentReference.attachment = 0;
	colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthStencilAttachmentReference = {};
	depthStencilAttachmentReference.attachment = 1;
	depthStencilAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpassDescription = {};
	subpassDescription.flags = 0;
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &colorAttachmentReference;
	subpassDescription.pDepthStencilAttachment = &depthStencilAttachmentReference;

	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.pNext = nullptr;
	renderPassCreateInfo.attachmentCount = 2;
	renderPassCreateInfo.pAttachments = attachmentDescriptions;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpassDescription;
	renderPassCreateInfo.dependencyCount = 0;
	renderPassCreateInfo.pDependencies = nullptr;

	if (vkCreateRenderPass(mEngine->getDevice(), &renderPassCreateInfo, nullptr, &mRenderPass) == VK_SUCCESS)
		return true;

	return false;
}
