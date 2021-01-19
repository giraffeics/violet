#ifndef GPUPIPELINE_H
#define GPUPIPELINE_H

#include <vulkan/vulkan.h>
#include <vector>

#include "GPUEngine.h"

class GPUPipeline
{
public:
	// constructors and destructor
	GPUPipeline(GPUEngine* engine, std::vector<std::string> shaderNames, std::vector<VkShaderStageFlagBits> shaderStages, VkRenderPass renderPass);
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
	VkRenderPass mRenderPass;
	VkPipelineLayout mPipelineLayout;
	VkPipeline mPipeline;
};

#endif