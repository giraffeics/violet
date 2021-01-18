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
* This class is responsible for preparing mesh instances for rendering;
* this means assembling all of the uniform data for each of their
* transforms, as well as any animation data, which will be used for
* rendering from all camera angles for a given frame.
*/
class GPUMeshWrangler : public GPUProcess
{
public:
	static constexpr size_t maxMeshInstances = 1024;

	// constructors and destructor
	GPUMeshWrangler(GPUEngine* engine);
	GPUMeshWrangler(GPUMeshWrangler& other) = delete;
	GPUMeshWrangler(GPUMeshWrangler&& other) = delete;
	GPUMeshWrangler& operator=(GPUMeshWrangler& other) = delete;
	~GPUMeshWrangler();

	// public functionality
	void reset();
	void stageMeshInstance(GPUMesh::Instance* instance);
	const std::vector<GPUMesh::Instance*> getMeshInstances() { return mMeshInstances; }
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

	GPUEngine* mEngine;

	// data used to assemble list of mesh instances for rendering
	glm::mat4* mUniformBufferData = nullptr;
	std::vector<GPUMesh::Instance*> mMeshInstances;
	size_t mNextInstance = 0;

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