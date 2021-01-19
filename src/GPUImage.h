#ifndef GPUIMAGE_H
#define GPUIMAGE_H

#include "GPUProcess.h"
#include <memory>

class GPUImage : public GPUProcess
{
public:
	// constructors & destructor
	GPUImage(VkFormatFeatureFlags requiredFeatures, VkImageUsageFlags usage, VkImageTiling tiling, size_t screenSizeMultiplier);
	GPUImage(VkFormatFeatureFlags requiredFeatures, VkImageUsageFlags usage, VkImageTiling tiling, size_t width, size_t height);
	GPUImage(GPUImage& other) = delete;
	GPUImage(GPUImage&& other) = delete;
	GPUImage& operator=(GPUImage& other) = delete;
	~GPUImage();

	// functions for setting up passable resource relationships
	const VkFormat* getFormatPTR() { return &mFormat; }
	const PassableResource<VkImageView>* getImageViewPR() { return mPRImageView.get(); }

	// virtual functions inherited from GPUProcess
	virtual void acquireLongtermResources();
	virtual void acquireFrameResources();
	virtual void cleanupFrameResources();
	virtual OperationType getOperationType();

private:
	// private member functions
	void allocateImage();
	void chooseImageFormat();
	void freeImage();
	std::vector<VkFormat> getFormatCandidates(VkFormatFeatureFlags requiredFeatures);

	// passable resources
	std::unique_ptr<PassableResource<VkImageView>> mPRImageView;

	// private member variables
	size_t mScreenSizeMultiplier = 1;
	size_t mWidth = 1;
	size_t mHeight = 1;
	bool mUseScreenSize = false;

	// owned vulkan handles
	VkImage mImage = VK_NULL_HANDLE;
	VkImageView mImageView = VK_NULL_HANDLE;
	VkDeviceMemory mImageMemory = VK_NULL_HANDLE;
	VkFormat mFormat = VK_FORMAT_MAX_ENUM;
	VkImageUsageFlags mUsage;
	VkFormatFeatureFlags mRequiredFeatures = 0;
	VkImageTiling mImageTiling;
};

#endif