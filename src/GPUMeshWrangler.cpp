#include "GPUMeshWrangler.h"

#include "glm_includes.h"
#include "GPUEngine.h"

GPUMeshWrangler::GPUMeshWrangler()
{
	mPRUniformBuffer = std::make_unique<GPUProcess::PassableResource<VkBuffer>>(this, &mUniformBuffer);
}

GPUMeshWrangler::~GPUMeshWrangler()
{
	VkDevice device = mEngine->getDevice();

	vkDestroyDescriptorPool(device, mDescriptorPool, nullptr);
	vkDestroyBuffer(device, mUniformBuffer, nullptr);
	vkDestroyBuffer(device, mTransferBuffer, nullptr);
	vkFreeMemory(device, mUniformBufferMemory, nullptr);
	vkFreeMemory(device, mTransferBufferMemory, nullptr);
}

/**
 * @brief Clears all staged mesh instance data so that instances can be staged for the next frame.
 */
void GPUMeshWrangler::reset()
{
	mMeshInstances.clear();
	mNextInstance = 0;
	mNextBufferMat4 = 0;
}

/**
 * @brief Stages a mesh instance for rendering.
 * 
 * Generates uniform data for the mesh instance and places it in an internal buffer so
 * that it can be transferred to GPU memory for use in render passes. Gives the mesh
 * instance an offset into the uniform buffer that can later be referenced when
 * rendering that mesh instance.
 * 
 * @param instance The mesh instance to be staged.
 */
void GPUMeshWrangler::stageMeshInstance(GPUMesh::Instance* instance)
{
	mMeshInstances.push_back(instance);
	mUniformBufferData[mNextBufferMat4] = instance->mTransform;
	instance->mDynamicOffset = sizeof(glm::mat4) * mNextBufferMat4;
	mNextInstance++;
	mNextBufferMat4 += mMinMat4sPerMeshInstance;
}

/**
 * @brief Returns a const vector of all currently staged mesh instances.
 * 
 * @return const std::vector<GPUMesh::Instance*> 
 */
const std::vector<GPUMesh::Instance*> GPUMeshWrangler::getMeshInstances()
{
	return mMeshInstances;
}

/**
 * @brief Binds the descriptor set and offset for a given mesh instance.
 * 
 * This enables the next draw command to use the mesh instance's uniform data.
 * 
 * @param commandBuffer Command buffer to record into.
 * @param bindPoint Pipeline bind point at which the descriptor set will be used.
 * @param pipelineLayout Pipeline layout used to program the binding.
 * @param instance Mesh instance for which the descriptor set is to be bound.
 */
void GPUMeshWrangler::bindModelDescriptor(VkCommandBuffer commandBuffer, VkPipelineBindPoint bindPoint, VkPipelineLayout pipelineLayout, GPUMesh::Instance* instance)
{
	vkCmdBindDescriptorSets(commandBuffer, bindPoint, pipelineLayout, 0, 1, &mDescriptorSet, 1, &(instance->mDynamicOffset));
}

/**
 * @brief Returns a const pointer to the PassableResource for this GPUMeshWrangler's uniform buffer.
 * 
 * @return const GPUProcess::PassableResource<VkBuffer>* 
 */
const GPUProcess::PassableResource<VkBuffer>* GPUMeshWrangler::getPRUniformBuffer()
{
	return mPRUniformBuffer.get();
}

void GPUMeshWrangler::acquireLongtermResources()
{
	mMinMat4sPerMeshInstance = mEngine->getPhysicalDeviceLimits()->minUniformBufferOffsetAlignment / sizeof(glm::mat4);
	if(mMinMat4sPerMeshInstance < 1)
		mMinMat4sPerMeshInstance = 1;

	createDescriptorPool();
	createDescriptorSet();
	createBuffers();

	VkDescriptorBufferInfo bufferInfo = {};
	bufferInfo.buffer = mUniformBuffer;
	bufferInfo.offset = 0;
	bufferInfo.range = maxBonesPerMesh * sizeof(glm::mat4);

	VkWriteDescriptorSet descriptorWrite = {};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.pNext = nullptr;
	descriptorWrite.dstSet = mDescriptorSet;
	descriptorWrite.dstBinding = 0;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	descriptorWrite.pBufferInfo = &bufferInfo;

	vkUpdateDescriptorSets(mEngine->getDevice(), 1, &descriptorWrite, 0, nullptr);

	mPRUniformBuffer->setPossibleValues({ mUniformBuffer });
}

// temporary testing variable
float rot = 0.0f;

VkCommandBuffer GPUMeshWrangler::performOperation(VkCommandPool commandPool)
{
	VkDevice device = mEngine->getDevice();

	// Set up a transfer operation using staged transform data
	VkCommandBuffer commandBuffer = mEngine->allocateCommandBuffer(commandPool);
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.pNext = nullptr;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = sizeof(glm::mat4) * maxMeshInstances;
	vkCmdCopyBuffer(commandBuffer, mTransferBuffer, mUniformBuffer, 1, &copyRegion);

	vkEndCommandBuffer(commandBuffer);

	return commandBuffer;
}

bool GPUMeshWrangler::createDescriptorPool()
{
	VkDevice device = mEngine->getDevice();

	VkDescriptorPoolSize poolSize = {};
	poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	poolSize.descriptorCount = 1;

	VkDescriptorPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	createInfo.maxSets = 1;
	createInfo.poolSizeCount = 1;
	createInfo.pPoolSizes = &poolSize;

	VkResult result = vkCreateDescriptorPool(device, &createInfo, nullptr, &mDescriptorPool);
	return (result == VK_SUCCESS);
}

bool GPUMeshWrangler::createDescriptorSet()
{
	VkDevice device = mEngine->getDevice();

	VkDescriptorSetLayout layout = mEngine->getModelDescriptorLayout();

	VkDescriptorSetAllocateInfo allocateInfo = {};
	allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocateInfo.pNext = nullptr;
	allocateInfo.descriptorPool = mDescriptorPool;
	allocateInfo.descriptorSetCount = 1;
	allocateInfo.pSetLayouts = &layout;

	return (vkAllocateDescriptorSets(device, &allocateInfo, &mDescriptorSet) == VK_SUCCESS);
}

bool GPUMeshWrangler::createBuffers()
{
	if (!mEngine->createBuffer(
		sizeof(glm::mat4) * maxMeshInstances,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mUniformBuffer, mUniformBufferMemory))
		return false;

	if (!mEngine->createBuffer(
		sizeof(glm::mat4) * maxMeshInstances,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		mTransferBuffer, mTransferBufferMemory))
		return false;

	vkMapMemory(mEngine->getDevice(), mTransferBufferMemory, 0, sizeof(glm::mat4) * maxMeshInstances, 0, (void**) &mUniformBufferData);

	/*
	// test by just rotating 45 degrees
	glm::vec3 axis = { 0.0, 1.0, 0.0 };
	glm::vec3 translation = { 0.0, 0.0, 0.5 };
	glm::mat4 transform = glm::translate(translation) * glm::rotate(glm::radians(210.0f), axis);

	mEngine->transferToBuffer(mUniformBuffer, &transform, sizeof(transform), 0);	//*/

	return true;
}