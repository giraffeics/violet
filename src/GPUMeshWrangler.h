#ifndef GPUMESHWRANGLER_H
#define GPUMESHWRANGLER_H

#include <memory>
#include <vector>
#include <vulkan/vulkan.h>

#include "GPUProcess.h"
#include "GPUMesh.h"
#include "glm_includes.h"

class GPUEngine;

/**
 * @brief Prepares active mesh instances to be rendered each frame.
 * 
 * Groups transform data for each mesh instance into a single large uniform buffer
 * and gives each mesh instance an offset into said buffer. This class's
 * responsibilities will likely expand as features are added to Violet.
 */
class GPUMeshWrangler : public GPUProcess
{
public:
	static constexpr size_t maxBonesPerMesh = 64;	// it could be more and still fit in the
													// minimum maxUniformBufferRange, but 64
													// seems a reasonable limit for now
	static constexpr size_t maxMeshInstances = 1024;

	// constructors and destructor
	GPUMeshWrangler();
	GPUMeshWrangler(GPUMeshWrangler& other) = delete;
	GPUMeshWrangler(GPUMeshWrangler&& other) = delete;
	GPUMeshWrangler& operator=(GPUMeshWrangler& other) = delete;
	~GPUMeshWrangler();

	// public functionality
	void reset();
	void stageMeshInstance(GPUMesh::Instance* instance);
	const std::vector<GPUMesh::Instance*> getMeshInstances();
	void bindModelDescriptor(VkCommandBuffer commandBuffer, VkPipelineBindPoint bindPoint, VkPipelineLayout pipelineLayout, GPUMesh::Instance* instance);

	// functions for setting up passable resource relationships
	const PassableResource<VkBuffer>* getPRUniformBuffer();

	// virtual functions inherited from GPUProcess
	virtual void acquireLongtermResources();
	virtual VkCommandBuffer performOperation(VkCommandPool commandPool);

private:
	bool createDescriptorPool();
	bool createDescriptorSet();
	bool createBuffers();

	// data used to assemble list of mesh instances for rendering
	glm::mat4* mUniformBufferData = nullptr;
	std::vector<GPUMesh::Instance*> mMeshInstances;
	size_t mNextInstance = 0;
	size_t mNextBufferMat4 = 0;
	size_t mMinMat4sPerMeshInstance = 256 / sizeof(glm::mat4);	// 256 = guaranteed maximum physical device
																// minUniformBufferOffsetAlignment as per VK spec

	// passable resources
	std::unique_ptr<PassableResource<VkBuffer>> mPRUniformBuffer;

	// Vulkan handles owned by GPUMeshWrangler
	VkDescriptorPool mDescriptorPool;
	VkDescriptorSet mDescriptorSet;
	VkBuffer mUniformBuffer;
	VkDeviceMemory mUniformBufferMemory;
	VkBuffer mTransferBuffer;
	VkDeviceMemory mTransferBufferMemory;
};

#endif