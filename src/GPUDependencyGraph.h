#ifndef GPUDEPENDENCYGRAPH_H
#define GPUDEPENDENCYGRAPH_H

#include <vector>
#include <unordered_map>
#include <vulkan/vulkan.h>

#include "GPUProcess.h"

class GPUEngine;

class GPUDependencyGraph
{
public:
	GPUDependencyGraph(GPUEngine* engine);
	~GPUDependencyGraph();
	void addProcess(GPUProcess* process);
	void build();
	void executeSequence();

private:
	// structs scoped to GPUDependencyGraph
	struct Node;

	struct Edge
	{
		size_t parentIndex;
		size_t childIndex;
		VkPipelineStageFlags pipelineStage;
		VkSemaphore signalSemaphore = VK_NULL_HANDLE;
	};

	struct Node
	{
		GPUProcess* process;
		uint32_t level;
		std::vector<size_t> backEdgeIndices;
		std::vector<size_t> forwardEdgeIndices;
		std::vector<VkSemaphore> waitSemaphores;
		std::vector<VkPipelineStageFlags> waitStages;
		std::vector<VkSemaphore> signalSemaphores;
	};

	struct SubmitGroup
	{
		std::vector<size_t> nodeIndices;
	};

	// private member functions
	void cleanupEdges();

	// private member variables
	GPUEngine* mEngine;
	std::vector<Node> mNodes;
	std::unordered_map<GPUProcess*, size_t> mProcessNodeIndices;
	std::vector<Edge> mEdges;
	std::vector<SubmitGroup> mSubmitSequence;
};

#endif