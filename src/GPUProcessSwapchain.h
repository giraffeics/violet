#ifndef GPUPROCESSSWAPCHAIN_H
#define GPUPROCESSSWAPCHAIN_H

#include "GPUProcess.h"
#include <memory>

class GPUProcessSwapchain : public GPUProcess
{
	// process that calls back to this class in order to present the acquired image
	friend class GPUProcessPresent;

public:
	// constructors and destructor
	GPUProcessSwapchain();
	GPUProcessSwapchain(GPUProcessSwapchain& other) = delete;
	GPUProcessSwapchain(GPUProcessSwapchain&& other) = delete;
	GPUProcessSwapchain& operator=(GPUProcessSwapchain& other) = delete;
	~GPUProcessSwapchain();

	// public getters
	GPUProcessPresent* getPresentProcess();
	const PassableImageView* getPRImageView();
	bool shouldRebuild();

	// virtual functions inherited from GPUProcess
	virtual OperationType getOperationType();
	virtual void acquireFrameResources();
	virtual void cleanupFrameResources();
	virtual void performOperation(std::vector<VkSemaphore> waitSemaphores, VkFence fence, VkSemaphore semaphore);

private:
	struct Frame
	{
		VkImageView imageView;
		VkImage image;
	};
	std::vector<Frame> mFrames;

	// private member functions
	bool chooseSurfaceFormat();
	bool createSwapchain();
	bool createFrames();
	void present(std::vector<VkSemaphore> waitSemaphores, VkFence fence, VkSemaphore semaphore);

	// member variables
	bool mShouldRebuild = false;
	VkSurfaceFormatKHR mSurfaceFormat;
	VkExtent2D mExtent;
	VkSwapchainKHR mSwapchain;
	uint32_t mCurrentImageIndex;
	std::unique_ptr<GPUProcessPresent> mPresentProcess;

	// member variables used for dependency passing
	VkImageView currentImageView;
	std::unique_ptr<GPUProcess::PassableImageView> mPRCurrentImageView;
};

class GPUProcessPresent : public GPUProcess
{
public:
	GPUProcessPresent(GPUProcessSwapchain* swapchainProcess);

	void setImageViewInPR(const PassableResource<VkImageView>* imageViewInPR);

	// virtual functions inherited from GPUProcess
	virtual OperationType getOperationType();
	virtual void performOperation(std::vector<VkSemaphore> waitSemaphores, VkFence fence, VkSemaphore semaphore);
	virtual std::vector<PRDependency> getPRDependencies();

private:
	GPUProcessSwapchain* mSwapchainProcess;
	const PassableResource<VkImageView>* mPRImageViewIn;
};

#endif