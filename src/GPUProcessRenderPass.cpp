#include "GPUProcessRenderPass.h"

#include "GPUEngine.h"
#include <glm/gtc/matrix_transform.hpp>

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

void GPUProcessRenderPass::setImageViewPR(const PassableResource<VkImageView>* prImageView)
{
	mPRImageView = prImageView;
}

void GPUProcessRenderPass::setUniformBufferPR(const PassableResource<VkBuffer>* prUniformBuffer)
{
	mPRUniformBuffer = prUniformBuffer;
}

void GPUProcessRenderPass::setImageFormatPTR(const VkFormat* format)
{
	mImageFormatPTR = format;
}

const GPUProcess::PassableResource<VkImageView>* GPUProcessRenderPass::getImageViewOutPR()
{
	return mPRImageViewOut.get();
}

std::vector<GPUProcess::PRDependency>  GPUProcessRenderPass::getPRDependencies()
{
	return std::vector<PRDependency>({ 
		{mPRImageView, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
		{mPRUniformBuffer, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT}
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
		VkPipelineLayout pipelineLayout = mPipeline->getLayout();
		GPUMeshWrangler* meshWrangler = mEngine->getMeshWrangler();
		glm::vec3 tvec = { 0.0f, 0.0f, -3.0f };
		auto extent = mEngine->getSurfaceExtent();
		glm::mat4 viewProjection = glm::perspective(45.0f, ((float)extent.width / (float)extent.height), 0.01f, 100.0f) * glm::translate(glm::identity<glm::mat4>(), tvec);

		VkClearValue colorClearValue = { 0.8f, 0.1f, 0.3f, 1.0f };

		VkRenderPassBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		beginInfo.pNext = nullptr;
		beginInfo.renderPass = mRenderPass;
		beginInfo.framebuffer = mFramebuffers.find(mCurrentImageView)->second;
		beginInfo.renderArea.offset = { 0, 0 };
		beginInfo.renderArea.extent = mEngine->getSurfaceExtent();
		beginInfo.clearValueCount = 1;
		beginInfo.pClearValues = &colorClearValue;
		vkCmdBeginRenderPass(commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

		mPipeline->bind(commandBuffer);
		vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &viewProjection);
		for (auto instance : meshWrangler->getMeshInstances())
		{
			meshWrangler->bindModelDescriptor(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, instance);
			instance->mMesh->draw(commandBuffer);
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
	for (auto imageViewUINT : possibleImageViews)
	{
		VkFramebuffer framebuffer;
		VkExtent2D extent = mEngine->getSurfaceExtent();
		VkImageView imageView = (VkImageView)imageViewUINT;

		VkFramebufferCreateInfo framebufferCreateInfo = {};
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.pNext = nullptr;
		framebufferCreateInfo.flags = 0;
		framebufferCreateInfo.renderPass = mRenderPass;
		framebufferCreateInfo.attachmentCount = 1;
		framebufferCreateInfo.pAttachments = &imageView;
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
	VkAttachmentDescription attachmentDescription = {};
	attachmentDescription.flags = 0;
	attachmentDescription.format = *mImageFormatPTR;
	attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentReference = {};
	colorAttachmentReference.attachment = 0;
	colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpassDescription = {};
	subpassDescription.flags = 0;
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &colorAttachmentReference;

	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.pNext = nullptr;
	renderPassCreateInfo.attachmentCount = 1;
	renderPassCreateInfo.pAttachments = &attachmentDescription;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpassDescription;
	renderPassCreateInfo.dependencyCount = 0;
	renderPassCreateInfo.pDependencies = nullptr;

	if (vkCreateRenderPass(mEngine->getDevice(), &renderPassCreateInfo, nullptr, &mRenderPass) == VK_SUCCESS)
		return true;

	return false;
}
