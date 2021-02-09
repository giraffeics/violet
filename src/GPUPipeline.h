#ifndef GPUPIPELINE_H
#define GPUPIPELINE_H

#include <vulkan/vulkan.h>
#include <vector>

#include "GPUEngine.h"
#include "GPUMesh.h"

/**
 * @brief Loads and manages a pipeline, its shaders, and associated resources.
 * 
 * Given a set of parameters in the constructor, GPUPipeline loads a set of compiled shaders
 * and creates a pipeline using them. The pipeline can then be bound in a VkCommandBuffer
 * using GPUPipeline::bind().
 */
class GPUPipeline
{
public:
	// constructors and destructor
	GPUPipeline(GPUEngine* engine, std::vector<std::string> shaderNames, std::vector<VkShaderStageFlagBits> shaderStages, 
				VkRenderPass renderPass, const std::vector<GPUMesh::AttributeType>& attributeTypes);
	GPUPipeline(GPUPipeline& other) = delete;
	GPUPipeline(GPUPipeline&& other) = delete;
	GPUPipeline& operator=(GPUPipeline& other) = delete;
	~GPUPipeline();

	// public getters & setters
	VkPipelineLayout getLayout() { return mPipelineLayout; }

	// public functionality
	bool valid();
	void invalidate();
	void validate();
	void bind(VkCommandBuffer commandBuffer);

private:
	// private member functions
	void buildShaderModules(std::vector<std::string>& shaderNames, std::vector<VkShaderStageFlagBits>& shaderStages);
	void buildPipelineLayout();
	void buildPipeline();

	// private member variables
	GPUEngine* mEngine;
	std::vector<VkShaderModule> mShaderModules;
	const char mEntryPointName[5] = "main";
	std::vector<VkPipelineShaderStageCreateInfo> mShaderStageCreateInfos;
	std::vector<GPUMesh::AttributeType> mAttributeTypes;
	VkRenderPass mRenderPass;
	VkPipelineLayout mPipelineLayout;
	VkPipeline mPipeline;
};

#endif