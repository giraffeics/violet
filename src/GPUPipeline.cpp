#include "GPUEngine.h"

#include <fstream>

#include "GPUMesh.h"

GPUPipeline::GPUPipeline(GPUEngine* engine, std::vector<std::string> shaderNames, std::vector<VkShaderStageFlagBits> shaderStages, VkRenderPass renderPass)
{
	mEngine = engine;
	mRenderPass = renderPass;

	buildShaderModules(shaderNames, shaderStages);

	// specify shader stages
	for (size_t i = 0; i < mShaderModules.size(); i++)
	{
		VkPipelineShaderStageCreateInfo createInfo;
		createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.stage = shaderStages[i];
		createInfo.module = mShaderModules[i];
		createInfo.pName = mEntryPointName;
		createInfo.pSpecializationInfo = nullptr;

		mShaderStageCreateInfos.push_back(createInfo);
	}

	buildPipelineLayout();
}

void GPUPipeline::buildShaderModules(std::vector<std::string>& shaderNames, std::vector<VkShaderStageFlagBits>& shaderStages)
{
	// compile each shader
	for (auto name : shaderNames)
	{
		VkShaderModule shaderModule;

		// read file
		std::ifstream infile("shaders/" + name + ".spv", std::ios::ate | std::ios::binary);
		if (!infile.is_open())
			return;
		size_t fileSize = infile.tellg();
		char* fileBuffer = new char[fileSize];
		infile.seekg(0);
		infile.read((char*)fileBuffer, fileSize);
		infile.close();

		// create shader module
		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.codeSize = fileSize;
		createInfo.pCode = reinterpret_cast<uint32_t*>(fileBuffer);
		vkCreateShaderModule(mEngine->getDevice(), &createInfo, nullptr, &shaderModule);

		// cleanup
		delete[] fileBuffer;

		// store in the vector of shader modules
		mShaderModules.push_back(shaderModule);
	}
}

void GPUPipeline::buildPipelineLayout()
{
	// grab model descriptor set layout handle
	VkDescriptorSetLayout modelLayout = mEngine->getModelDescriptorLayout();

	VkPushConstantRange pushConstantRange = {};
	pushConstantRange.offset = 0;
	pushConstantRange.size = 64; // size of glm::mat4
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkPipelineLayoutCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.setLayoutCount = 1;
	createInfo.pSetLayouts = &modelLayout;
	createInfo.pushConstantRangeCount = 1;
	createInfo.pPushConstantRanges = &pushConstantRange;

	if (vkCreatePipelineLayout(mEngine->getDevice(), &createInfo, nullptr, &mPipelineLayout) != VK_SUCCESS)
		return;
}

void GPUPipeline::buildPipeline()
{
	// specify fixed-function details
	// TODO: make this more versatile
	VkVertexInputAttributeDescription attribDescription = {};
	attribDescription.binding = 0;
	attribDescription.location = 0;
	attribDescription.offset = 0;
	VkVertexInputBindingDescription bindingDescription = {};
	bindingDescription.binding = 0;
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	GPUMesh::getAttributeProperties(bindingDescription.stride, attribDescription.format, GPUMesh::MESH_ATTRIBUTE_POSITION);

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.pNext = nullptr;
	vertexInputInfo.flags = 0;
	vertexInputInfo.vertexAttributeDescriptionCount = 1;
	vertexInputInfo.pVertexAttributeDescriptions = &attribDescription;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;

	VkPipelineInputAssemblyStateCreateInfo assemblyInfo = {};
	assemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	assemblyInfo.pNext = nullptr;
	assemblyInfo.flags = 0;
	assemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	assemblyInfo.primitiveRestartEnable = VK_FALSE;

	VkExtent2D extent = mEngine->getSurfaceExtent();

	VkViewport viewport = {};
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = extent.width;
	viewport.height = extent.height;
	viewport.minDepth = 0.0;
	viewport.maxDepth = 1.0;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = extent;

	VkPipelineViewportStateCreateInfo viewportInfo = {};
	viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportInfo.pNext = nullptr;
	viewportInfo.flags = 0;
	viewportInfo.viewportCount = 1;
	viewportInfo.pViewports = &viewport;
	viewportInfo.scissorCount = 1;
	viewportInfo.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizationInfo = {};
	rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationInfo.pNext = nullptr;
	rasterizationInfo.flags = 0;
	rasterizationInfo.depthClampEnable = VK_FALSE;
	rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizationInfo.depthBiasEnable = VK_FALSE;
	rasterizationInfo.lineWidth = 1.0f;

	VkPipelineMultisampleStateCreateInfo multisampleInfo = {};
	multisampleInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleInfo.pNext = nullptr;
	multisampleInfo.flags = 0;
	multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampleInfo.sampleShadingEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState colorBlendAttachmentInfo = {};
	colorBlendAttachmentInfo.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
		| VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachmentInfo.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlendInfo = {};
	colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendInfo.pNext = nullptr;
	colorBlendInfo.flags = 0;
	colorBlendInfo.logicOpEnable = VK_FALSE;
	colorBlendInfo.attachmentCount = 1;
	colorBlendInfo.pAttachments = &colorBlendAttachmentInfo;

	VkPipelineDepthStencilStateCreateInfo depthStencilInfo = {};
	depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilInfo.pNext = nullptr;
	depthStencilInfo.flags = 0;
	depthStencilInfo.depthTestEnable = VK_TRUE;
	depthStencilInfo.depthWriteEnable = VK_TRUE;
	depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
	depthStencilInfo.stencilTestEnable = VK_FALSE;

	// create graphics pipeline
	VkGraphicsPipelineCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.stageCount = mShaderStageCreateInfos.size();
	createInfo.pStages = mShaderStageCreateInfos.data();
	createInfo.pVertexInputState = &vertexInputInfo;
	createInfo.pInputAssemblyState = &assemblyInfo;
	createInfo.pTessellationState = nullptr;
	createInfo.pViewportState = &viewportInfo;
	createInfo.pRasterizationState = &rasterizationInfo;
	createInfo.pMultisampleState = &multisampleInfo;
	createInfo.pDepthStencilState = &depthStencilInfo;
	createInfo.pColorBlendState = &colorBlendInfo;
	createInfo.pDynamicState = nullptr;
	createInfo.layout = mPipelineLayout;
	createInfo.renderPass = mRenderPass;
	createInfo.subpass = 0;
	createInfo.basePipelineHandle = VK_NULL_HANDLE;
	createInfo.basePipelineIndex = 0;

	vkCreateGraphicsPipelines(mEngine->getDevice(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &mPipeline);
}

GPUPipeline::~GPUPipeline()
{
	if (valid())
		invalidate();

	VkDevice device = mEngine->getDevice();

	vkDestroyPipelineLayout(device, mPipelineLayout, nullptr);

	for (VkShaderModule module : mShaderModules)
	{
		vkDestroyShaderModule(device, module, nullptr);
	}
}

void GPUPipeline::bind(VkCommandBuffer commandBuffer)
{
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);
}

// TODO: implement to check validity
bool GPUPipeline::valid()
{
	return (mPipeline != VK_NULL_HANDLE);
}

void GPUPipeline::invalidate()
{
	vkDestroyPipeline(mEngine->getDevice(), mPipeline, nullptr);
	mPipeline = VK_NULL_HANDLE;
}

void GPUPipeline::validate()
{
	buildPipeline();
}