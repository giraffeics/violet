#include "GPUDependencyGraph.h"

#include "GPUEngine.h"

GPUDependencyGraph::GPUDependencyGraph(GPUEngine* engine)
{
	mEngine = engine;
}

GPUDependencyGraph::~GPUDependencyGraph()
{
	cleanupEdges();

	for (auto& entry : mProcessNodeIndices)
	{
		delete entry.first;
	}
}

/**
 * @brief Adds a process to this dependency graph.
 * 
 * No validation is performed when addProcess() is called, and the
 * order in which processes are added does not matter. This
 * GPUDependencyGraph takes direct ownership of process. If process
 * depends on any other GPUProcess instances, those must
 * also be explicitly added to this GPUDependencyGraph.
 * 
 * @param process The GPUProcess instance to be added to this dependency graph.
 */
void GPUDependencyGraph::addProcess(GPUProcess* process)
{
	// add a node to the list
	size_t index = mNodes.size();
	mNodes.push_back({process,0});
	mProcessNodeIndices.insert({ process, index });
}

/**
 * @brief Validates and builds this dependency graph.
 * 
 * Creates edges connecting all processes which depend on each other.
 * Acquires all resources needed to execute the sequence of processes.
 */
void GPUDependencyGraph::build()
{
	// first, create all necessary edges
	cleanupEdges();
	for (size_t i=0; i<mNodes.size(); i++)
	{
		auto& node = mNodes[i];
		auto dependencies = node.process->getPRDependencies();

		// create edge for each PRDependency
		for (auto& dependency : dependencies)
		{
			GPUProcess* parentPTR = dependency.resource->getSourceProcess();
			size_t parentIndex = mProcessNodeIndices.find(parentPTR)->second;
			VkSemaphore semaphore = VK_NULL_HANDLE;
			if (parentPTR->getOperationType() != GPUProcess::OP_TYPE_NOOP)
				semaphore = mEngine->createSemaphore();
			// TODO: validate parent index
			size_t edgeIndex = mEdges.size();
			mEdges.push_back({
				parentIndex,
				i,
				dependency.pipelineStage,
				semaphore	// each edge owns one VkSemaphore
				});
			node.backEdgeIndices.push_back(edgeIndex);
			mNodes[parentIndex].forwardEdgeIndices.push_back(edgeIndex);
		}
	}

	// fill each node's signalSemaphores and signalStages vectors,
	// as well as waitSemaphores and waitStages
	for (auto& node : mNodes)
	{
		node.waitSemaphores.clear();
		node.waitStages.clear();
		node.signalSemaphores.clear();

		for (auto& edgeIndex : node.forwardEdgeIndices)
		{
			auto& edge = mEdges[edgeIndex];
			if (edge.signalSemaphore != VK_NULL_HANDLE)
				node.signalSemaphores.push_back(edge.signalSemaphore);
		}

		for (auto& edgeIndex : node.backEdgeIndices)
		{
			auto& edge = mEdges[edgeIndex];
			if (edge.signalSemaphore != VK_NULL_HANDLE)
			{
				node.waitSemaphores.push_back(edge.signalSemaphore);
				node.waitStages.push_back(edge.pipelineStage);
			}
		}
	}

	// calculate each node's level (longest dependency chain)
	// using modified bellman-ford algorithm
	for (size_t i = 0; i < mNodes.size(); i++)
	{
		for (auto& node : mNodes)
		{
			for (size_t edgeIndex : node.backEdgeIndices)
			{
				auto& parent = mNodes[mEdges[edgeIndex].parentIndex];
				if (parent.level + 1 > node.level)
					node.level = parent.level + 1;
			}
		}
	}

	// find number of levels
	size_t maxLevel = 0;
	for (auto& node : mNodes)
		if (node.level > maxLevel)
			maxLevel = node.level;

	// fill levels
	mSubmitSequence.clear();
	mSubmitSequence.resize(maxLevel+1);
	for (size_t i = 0; i < mNodes.size(); i++)
	{
		auto& node = mNodes[i];
		mSubmitSequence[node.level].nodeIndices.push_back(i);
	}

	// finally, acquire longterm resources for each process
	for (auto& group : mSubmitSequence)
	{
		for (auto i : group.nodeIndices)
		{
			mNodes[i].process->acquireLongtermResources();
			mNodes[i].process->acquireFrameResources();
		}
	}
}

/**
 * @brief Invalidates all resources which depend on the VkSurface.
 * 
 * If those resources must be reallocated, then they are freed when
 * invalidateFrameResources() is called. This GPUDependencyGraph cannot be
 * executed again until validateFrameResources() is called.
 */
void GPUDependencyGraph::invalidateFrameResources()
{
	// The iteration here needs to be done this way to avoid integer overflow
	for (size_t i = mSubmitSequence.size(); i > 0;)
	{
		i--;

		for (auto i : mSubmitSequence[i].nodeIndices)
		{
			mNodes[i].process->cleanupFrameResources();
		}
	}
}

/**
 * @brief Acquires and/or validates all resources which depend on the VkSurface.
 * 
 * After acquireFrameResources() is called, this GPUDependencyGraph() can be executed again.
 */
void GPUDependencyGraph::acquireFrameResources()
{
	for (auto& group : mSubmitSequence)
	{
		for (auto i : group.nodeIndices)
		{
			mNodes[i].process->acquireFrameResources();
		}
	}
}

// TODO: batch multiple command buffers into one VkSubmitInfo where appropriate
// (won't make a difference until more complex rendering is implemented)
// TODO: support batching into non-graphics command queues
/**
 * @brief Executes the sequence of processes in this GPUDependencyGraph.
 */
void GPUDependencyGraph::executeSequence()
{
	std::vector<VkCommandBuffer> createdCommandBuffers;

	for (auto& group : mSubmitSequence)
	{
		// figure out number of submits
		size_t numSubmits = 0;
		for (size_t i : group.nodeIndices)
		{
			auto& node = mNodes[i];
			if (node.process->getOperationType() == GPUProcess::OP_TYPE_COMMAND)
				numSubmits++;
		}

		// perform tasks for each process
		std::vector<VkSubmitInfo> submitInfos(numSubmits);
		std::vector<VkCommandBuffer> commandBuffers(numSubmits);
		size_t currentSubmit = 0;

		bool opFailed = false;

		for (size_t i : group.nodeIndices)
		{
			if(opFailed)
				break;

			auto& node = mNodes[i];
			switch(node.process->getOperationType())
			{
			case GPUProcess::OP_TYPE_COMMAND:
				{
					commandBuffers[currentSubmit] = node.process->performOperation(mEngine->getGraphicsPool());
					createdCommandBuffers.push_back(commandBuffers[currentSubmit]);

					auto& submitInfo = submitInfos[currentSubmit];
					submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
					submitInfo.pNext = nullptr;
					submitInfo.waitSemaphoreCount = node.waitSemaphores.size();
					submitInfo.pWaitSemaphores = node.waitSemaphores.data();
					submitInfo.pWaitDstStageMask = node.waitStages.data();
					submitInfo.commandBufferCount = 1;
					submitInfo.pCommandBuffers = &commandBuffers[currentSubmit];
					submitInfo.signalSemaphoreCount = node.signalSemaphores.size();
					submitInfo.pSignalSemaphores = node.signalSemaphores.data();

					currentSubmit++;
					break;
				}
			case GPUProcess::OP_TYPE_OTHER:
				{
					VkSemaphore signalSemaphore = VK_NULL_HANDLE;
					if (node.signalSemaphores.size() > 0)
						signalSemaphore = node.signalSemaphores[0];

					if(!node.process->performOperation(node.waitSemaphores, VK_NULL_HANDLE, signalSemaphore))
						opFailed = true;	// abort dependency graph execution
					break;
				}
			}
		}

		if(opFailed)
			break;

		// submit command buffers
		VkResult result = vkQueueSubmit(mEngine->getGraphicsQueue(), submitInfos.size(), submitInfos.data(), VK_NULL_HANDLE);
		if(result != VK_SUCCESS)
			break;	// abort dependency graph execution
	}

	// TODO: implement better syncronization
	vkQueueWaitIdle(mEngine->getGraphicsQueue());
	vkQueueWaitIdle(mEngine->getPresentQueue());

	// TODO: implement better collection of command buffers to avoid unnecessary vector reallocation
	vkFreeCommandBuffers(mEngine->getDevice(), mEngine->getGraphicsPool(), createdCommandBuffers.size(), createdCommandBuffers.data());

	vkResetCommandPool(mEngine->getDevice(), mEngine->getGraphicsPool(), 0);
}

/**
 * @brief Frees all edges and their associated syncronization objects.
 */
void GPUDependencyGraph::cleanupEdges()
{
	for (auto& edge : mEdges)
		vkDestroySemaphore(mEngine->getDevice(), edge.signalSemaphore, nullptr);
	mEdges.clear();
}