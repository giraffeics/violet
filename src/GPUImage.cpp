#include "GPUImage.h"

#include "GPUEngine.h"

GPUImage::GPUImage(VkFormatFeatureFlags requiredFeatures, VkImageUsageFlags usage, VkImageTiling tiling, size_t screenSizeMultiplier)
{
	mRequiredFeatures = requiredFeatures;
	mImageTiling = tiling;
	mUsage = usage;

	mScreenSizeMultiplier = screenSizeMultiplier;
	mUseScreenSize = true;

	mPRImageView = std::make_unique<PassableResource<VkImageView>>(this, &mImageView);
}

GPUImage::GPUImage(VkFormatFeatureFlags requiredFeatures, VkImageUsageFlags usage, VkImageTiling tiling, size_t width, size_t height)
{
	mRequiredFeatures = requiredFeatures;
	mImageTiling = tiling;
	mUsage = usage;

	mWidth = width;
	mHeight = height;
	mUseScreenSize = false;

	mPRImageView = std::make_unique<PassableResource<VkImageView>>(this, &mImageView);
}

GPUImage::~GPUImage()
{
	freeImage();
}

void GPUImage::acquireLongtermResources()
{
	chooseImageFormat();

	if (!mUseScreenSize)
		allocateImage();
}

void GPUImage::acquireFrameResources()
{
	if (mUseScreenSize)
		allocateImage();
}

void GPUImage::cleanupFrameResources()
{
	if (mUseScreenSize)
		freeImage();
}

GPUProcess::OperationType GPUImage::getOperationType()
{
	return OP_TYPE_NOOP;
}

void GPUImage::allocateImage()
{
	if (mUseScreenSize)
	{
		auto extent = mEngine->getSurfaceExtent();
		mWidth = extent.width * mScreenSizeMultiplier;
		mHeight = extent.height * mScreenSizeMultiplier;
	}

	VkDevice device = mEngine->getDevice();

	// choose aspect flags
	VkImageAspectFlags aspectMask = 0;
	if (mUsage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
		aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	if (mUsage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
		aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

	// create image
	{
		VkImageCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.imageType = VK_IMAGE_TYPE_2D;
		createInfo.format = mFormat;
		createInfo.extent.width = mWidth;
		createInfo.extent.height = mHeight;
		createInfo.extent.depth = 1;
		createInfo.mipLevels = 1;
		createInfo.arrayLayers = 1;
		createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		createInfo.tiling = mImageTiling;
		createInfo.usage = mUsage;
		createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		vkCreateImage(device, &createInfo, nullptr, &mImage);
	}

	// allocate memory
	{
		VkMemoryRequirements memoryRequirements;
		vkGetImageMemoryRequirements(device, mImage, &memoryRequirements);

		VkMemoryAllocateInfo allocateInfo = {};
		allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocateInfo.pNext = nullptr;
		allocateInfo.memoryTypeIndex = mEngine->findMemoryType(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		allocateInfo.allocationSize = memoryRequirements.size;
		vkAllocateMemory(device, &allocateInfo, nullptr, &mImageMemory);

		vkBindImageMemory(device, mImage, mImageMemory, 0);
	}

	// create image view
	{
		VkImageViewCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.image = mImage;
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = mFormat;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_R;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_G;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_B;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_A;
		createInfo.subresourceRange.aspectMask = aspectMask;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;
		createInfo.subresourceRange.levelCount = 1;

		vkCreateImageView(device, &createInfo, nullptr, &mImageView);
	}

	// set passable resource possible values
	mPRImageView->setPossibleValues({mImageView});
}

void GPUImage::chooseImageFormat()
{
	// get a list of format candidates
	std::vector<VkFormat> formatCandidates;
	// TODO: enhance these candidate lists
	if (mRequiredFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
	{
		formatCandidates = {
			VK_FORMAT_D32_SFLOAT,
			VK_FORMAT_X8_D24_UNORM_PACK32,
			VK_FORMAT_D32_SFLOAT_S8_UINT,
			VK_FORMAT_D24_UNORM_S8_UINT,
			VK_FORMAT_D16_UNORM
		};
	}
	else if (mRequiredFeatures & (VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT))
	{
		formatCandidates = {
			VK_FORMAT_R8G8B8A8_UNORM
		};
	}

	VkPhysicalDevice physicalDevice = mEngine->getPhysicalDevice();

	for (VkFormat format : formatCandidates)
	{
		VkFormatProperties properties;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &properties);

		if (mImageTiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & mRequiredFeatures) == mRequiredFeatures)
		{
			mFormat = format;
			return;
		}
		if (mImageTiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & mRequiredFeatures) == mRequiredFeatures)
		{
			mFormat = format;
			return;
		}
	}
}

void GPUImage::freeImage()
{
	VkDevice device = mEngine->getDevice();

	vkDestroyImageView(device, mImageView, nullptr);
	vkDestroyImage(device, mImage, nullptr);
	vkFreeMemory(device, mImageMemory, nullptr);
}
