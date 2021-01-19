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

void GPUDependencyGraph::addProcess(GPUProcess* process)
{
	// add a node to the list
	size_t index = mNodes.size();
	mNodes.push_back({process,0});
	mProcessNodeIndices.insert({ process, index });
}

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

		for (size_t i : group.nodeIndices)
		{
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

					node.process->performOperation(node.waitSemaphores, VK_NULL_HANDLE, signalSemaphore);
					break;
				}
			}
		}

		// submit command buffers
		vkQueueSubmit(mEngine->getGraphicsQueue(), submitInfos.size(), submitInfos.data(), VK_NULL_HANDLE);
	}

	// TODO: implement better syncronization
	vkQueueWaitIdle(mEngine->getGraphicsQueue());
	vkQueueWaitIdle(mEngine->getPresentQueue());

	// TODO: implement better collection of command buffers to avoid unnecessary vector reallocation
	vkFreeCommandBuffers(mEngine->getDevice(), mEngine->getGraphicsPool(), createdCommandBuffers.size(), createdCommandBuffers.data());

	vkResetCommandPool(mEngine->getDevice(), mEngine->getGraphicsPool(), 0);
}

void GPUDependencyGraph::cleanupEdges()
{
	for (auto& edge : mEdges)
		vkDestroySemaphore(mEngine->getDevice(), edge.signalSemaphore, nullptr);
	mEdges.clear();
}