#ifndef GPUPROCESSSWAPCHAIN_H
#define GPUPROCESSSWAPCHAIN_H

#include "GPUProcess.h"
#include <memory>

class GPUProcessPresent;

/**
 * @brief A GPUProcess that manages swapchain resources and acquires swapchain images.
 * 
 * Each GPUProcessSwapchain owns a PassableImageView representing the swapchain image
 * for the current frame. This must be passed to the final render pass process as a
 * color attachment, so that the results of rendering can be presented. The final render
 * pass must then pass the same ImageView to this GPUProcessSwapchain's corresponding
 * GPUProcessPresent.
 * 
 * Each GPUProcessSwapchain creates, and holds a pointer to, a corresponding GPUProcessPresent.
 * Both of these processes are explicitly added to the dependency graph by the GPUEngine.
 */
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
	virtual bool performOperation(std::vector<VkSemaphore> waitSemaphores, VkFence fence, VkSemaphore semaphore);

private:
	struct Frame
	{
		VkImageView imageView;
		VkImage image;
	};
	std::vector<Frame> mFrames;

	// timeout in ns for acquiring images
	static constexpr uint64_t imageTimeout = 1000000000;

	// private member functions
	bool chooseSurfaceFormat();
	bool createSwapchain();
	bool createFrames();
	bool present(std::vector<VkSemaphore> waitSemaphores, VkFence fence, VkSemaphore semaphore);

	// member variables
	bool mShouldRebuild = false;
	VkSurfaceFormatKHR mSurfaceFormat;
	VkExtent2D mExtent;
	VkSwapchainKHR mSwapchain;
	uint32_t mCurrentImageIndex;
	GPUProcessPresent* mPresentProcess;

	// member variables used for dependency passing
	VkImageView currentImageView;
	std::unique_ptr<GPUProcess::PassableImageView> mPRCurrentImageView;
};

/**
 * @brief A GPUProcess that works with GPUProcessSwapchain to present images to the surface.
 * 
 * Every GPUProcessSwapchain instance creates, and holds a pointer to, a GPUProcessPresent.
 * Both must be explicitly added to the GPUDependencyGraph.
 * Every GPUProcessPresent holds a pointer to its corresponding GPUProcessSwapchain, which it
 * uses to perform its operation.
 * 
 * The GPUEngine creates a GPUProcessSwapchain and holds a reference to both it and its
 * corresponding GPUProcessPresent. The final render pass process must pass its color
 * attachment to the GPUEngine's GPUProcessPresent, so that the image can be presented.
 */
class GPUProcessPresent : public GPUProcess
{
public:
	GPUProcessPresent(GPUProcessSwapchain* swapchainProcess);

	void setImageViewInPR(const PassableResource<VkImageView>* imageViewInPR);

	// virtual functions inherited from GPUProcess
	virtual OperationType getOperationType();
	virtual bool performOperation(std::vector<VkSemaphore> waitSemaphores, VkFence fence, VkSemaphore semaphore);
	virtual std::vector<PRDependency> getPRDependencies();

private:
	GPUProcessSwapchain* mSwapchainProcess;
	const PassableResource<VkImageView>* mPRImageViewIn;
};

#endif
