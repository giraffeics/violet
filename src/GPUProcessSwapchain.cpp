#include "GPUProcessSwapchain.h"

#include "GPUEngine.h"

// GPUProcessSwapchain implementation

GPUProcessSwapchain::GPUProcessSwapchain()
{
	mPRCurrentImageView = std::make_unique<PassableResource<VkImageView>>(this, &currentImageView);
	mPresentProcess = std::make_unique<GPUProcessPresent>(this);
}

GPUProcessSwapchain::~GPUProcessSwapchain()
{
	VkDevice device = mEngine->getDevice();

	for (auto& frame : mFrames)
	{
		vkDestroyImageView(device, frame.imageView, nullptr);
	}

	vkDestroySwapchainKHR(device, mSwapchain, nullptr);
}

GPUProcessPresent* GPUProcessSwapchain::getPresentProcess()
{
	return mPresentProcess.get();
}

const GPUProcess::PassableResource<VkImageView>* GPUProcessSwapchain::getPRImageView()
{
	return mPRCurrentImageView.get();
}

const VkFormat* GPUProcessSwapchain::getImageFormatPTR()
{
	return &(mSurfaceFormat.format);
}

bool GPUProcessSwapchain::isOperationCommand()
{
	return false;
}

void GPUProcessSwapchain::acquireLongtermResources()
{
	if (!chooseSurfaceFormat())
		return;
	if (!createSwapchain())
		return;
	if (!createFrames())
		return;
}

bool GPUProcessSwapchain::createSwapchain()
{
	// grab needed VK handles from mEngine
	VkPhysicalDevice physicalDevice = mEngine->getPhysicalDevice();
	VkDevice device = mEngine->getDevice();
	VkSurfaceKHR surface = mEngine->getSurface();
	VkExtent2D extent = mEngine->getSurfaceExtent();
	uint32_t graphicsFamily = mEngine->getGraphicsQueueFamily();
	uint32_t presentFamily = mEngine->getPresentQueueFamily();

	// create swapchain
	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.surface = surface;
	createInfo.minImageCount = 2;
	createInfo.imageFormat = mSurfaceFormat.format;
	createInfo.imageColorSpace = mSurfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	uint32_t queueFamilyIndices[] = { graphicsFamily, presentFamily };
	if (graphicsFamily == presentFamily)
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
	}
	else
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	return (vkCreateSwapchainKHR(device, &createInfo, nullptr, &mSwapchain) == VK_SUCCESS);
}

bool GPUProcessSwapchain::createFrames()
{
	// grab needed handles from engine
	VkDevice device = mEngine->getDevice();

	// query for swapchain images
	uint32_t numSwapchainImages;
	vkGetSwapchainImagesKHR(device, mSwapchain, &numSwapchainImages, nullptr);
	std::vector<VkImage> imagesVector(numSwapchainImages);
	vkGetSwapchainImagesKHR(device, mSwapchain, &numSwapchainImages, imagesVector.data());

	// clear vectors
	mFrames.clear();
	std::vector<VkImageView> imageViewPossibleValues;

	// create frames
	for (uint32_t i = 0; i < numSwapchainImages; i++)
	{
		// create image view
		auto image = imagesVector[i];
		VkImageView imageView;

		VkImageViewCreateInfo imageViewCreateInfo = {};
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.pNext = nullptr;
		imageViewCreateInfo.flags = 0;
		imageViewCreateInfo.image = image;
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfo.format = mSurfaceFormat.format;
		imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.layerCount = 1;

		vkCreateImageView(device, &imageViewCreateInfo, nullptr, &imageView);

		// add frame
		mFrames.push_back({ imageView, image });
		imageViewPossibleValues.push_back(imageView);
	}

	// store possible values in PassableResource
	mPRCurrentImageView->setPossibleValues(imageViewPossibleValues);

	return true;
}

bool GPUProcessSwapchain::chooseSurfaceFormat()
{
	VkPhysicalDevice physicalDevice = mEngine->getPhysicalDevice();
	VkSurfaceKHR surface = mEngine->getSurface();

	// query available formats
	uint32_t numFormats;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &numFormats, nullptr);
	std::vector<VkSurfaceFormatKHR> formats(numFormats);
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &numFormats, formats.data());

	if (numFormats == 0)
		return false;

	int bestRating = -1;
	VkSurfaceFormatKHR bestFormat;
	for (auto& format : formats)
	{
		int rating = 0;

		if (format.format == VK_FORMAT_B8G8R8A8_SRGB)
			rating += 2;

		if (format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			rating += 1;

		if (rating > bestRating)
			bestFormat = format;
	}

	mSurfaceFormat = bestFormat;
	return true;
}

void GPUProcessSwapchain::performOperation(std::vector<VkSemaphore> waitSemaphores, VkFence fence, VkSemaphore semaphore)
{
	vkAcquireNextImageKHR(mEngine->getDevice(), mSwapchain, std::numeric_limits<uint64_t>::max(), semaphore, fence, &mCurrentImageIndex);
	currentImageView = mFrames[mCurrentImageIndex].imageView;
}

void GPUProcessSwapchain::present(std::vector<VkSemaphore> waitSemaphores, VkFence fence, VkSemaphore semaphore)
{
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = nullptr;
	presentInfo.waitSemaphoreCount = waitSemaphores.size();
	presentInfo.pWaitSemaphores = waitSemaphores.data();
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &mSwapchain;
	presentInfo.pImageIndices = &mCurrentImageIndex;
	presentInfo.pResults = nullptr;
	vkQueuePresentKHR(mEngine->getPresentQueue(), &presentInfo);
}

// GPUProcessPresent implementation

GPUProcessPresent::GPUProcessPresent(GPUProcessSwapchain* swapchainProcess)
{
	mSwapchainProcess = swapchainProcess;
}

void GPUProcessPresent::setImageViewInPR(const PassableResource<VkImageView>* imageViewInPR)
{
	mPRImageViewIn = imageViewInPR;
}

bool GPUProcessPresent::isOperationCommand()
{
	return false;
}

void GPUProcessPresent::performOperation(std::vector<VkSemaphore> waitSemaphores, VkFence fence, VkSemaphore semaphore)
{
	mSwapchainProcess->present(waitSemaphores, fence, semaphore);
}

std::vector<GPUProcess::PRDependency> GPUProcessPresent::getPRDependencies()
{
	return { {mPRImageViewIn} };
}