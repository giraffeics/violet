#include "GPUEngine.h"

#include <iostream>
#include <limits>

GPUEngine::GPUEngine(const std::vector<GPUProcess*>& processes, GPUWindowSystem* windowSystem, std::string appName, std::string engineName, uint32_t appVersion, uint32_t engineVersion)
{
	// check to see if windowSystem is in processes
	std::vector<GPUProcess*> lProcesses(processes);
	bool windowSystemFound = false;
	for (auto process : lProcesses)
		if (process == windowSystem)
		{
			windowSystemFound = true;
			break;
		}
	if (!windowSystemFound)
		lProcesses.push_back(windowSystem);

	mWindowSystem = windowSystem;

	auto instanceExtensions = createInstanceExtensionsVector(lProcesses);
	auto deviceExtensions = createDeviceExtensionsVector(lProcesses);

	if(createInstance(instanceExtensions, appName, engineName, appVersion, engineVersion))
		std::cout << "Instance created successfully!!" << std::endl;
	else
		std::cout << "Could not create instance!!" << std::endl;

	if (createSurface())
		std::cout << "Surface created successfully!!" << std::endl;
	else
		std::cout << "Could not create surface!!" << std::endl;

	if (choosePhysicalDevice(deviceExtensions))
		std::cout << "Physical device selected successfully!!" << std::endl;
	else
		std::cout << "Could not find a suitable physical device!!" << std::endl;

	if (createLogicalDevice(deviceExtensions))
		std::cout << "Locigal device created successfully!!" << std::endl;
	else
		std::cout << "Could not create logical device!!" << std::endl;

	if (createCommandPools())
		std::cout << "Command pools created successfully!!" << std::endl;
	else
		std::cout << "Could not create command pools!!" << std::endl;

	if (createSwapchain())
		std::cout << "Swapchain created successfully!!" << std::endl;
	else
		std::cout << "Could not create swapchain!!" << std::endl;

	if (createRenderPass())
		std::cout << "Render pass created successfully!!" << std::endl;
	else
		std::cout << "Could not create render pass!!" << std::endl;

	createFrames();
}

bool GPUEngine::createSurface()
{
	mSurface = mWindowSystem->createSurface(mInstance);
	if (mSurface == VK_NULL_HANDLE)
		return false;

	mSurfaceExtent = mWindowSystem->getSurfaceExtent();
	return true;
}

bool GPUEngine::createRenderPass()
{
	// create render pass
	VkRenderPass renderPass;

	VkAttachmentDescription attachmentDescription = {};
	attachmentDescription.flags = 0;
	attachmentDescription.format = mSurfaceFormat.format;
	attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentReference = {};
	colorAttachmentReference.attachment = 0;
	colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpassDescription = {};
	subpassDescription.flags = 0;
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &colorAttachmentReference;

	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.pNext = nullptr;
	renderPassCreateInfo.attachmentCount = 1;
	renderPassCreateInfo.pAttachments = &attachmentDescription;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpassDescription;
	renderPassCreateInfo.dependencyCount = 0;
	renderPassCreateInfo.pDependencies = nullptr;

	if (vkCreateRenderPass(mLogicalDevice, &renderPassCreateInfo, nullptr, &mRenderPass) == VK_SUCCESS)
		return true;

	return false;
}

bool GPUEngine::createFrames()
{
	// query for swapchain images
	uint32_t numSwapchainImages;
	vkGetSwapchainImagesKHR(mLogicalDevice, mSwapchain, &numSwapchainImages, nullptr);
	std::vector<VkImage> imagesVector(numSwapchainImages);
	vkGetSwapchainImagesKHR(mLogicalDevice, mSwapchain, &numSwapchainImages, imagesVector.data());

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

		vkCreateImageView(mLogicalDevice, &imageViewCreateInfo, nullptr, &imageView);

		// create framebuffer
		VkFramebuffer framebuffer;

		VkFramebufferCreateInfo framebufferCreateInfo = {};
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.pNext = nullptr;
		framebufferCreateInfo.flags = 0;
		framebufferCreateInfo.renderPass = mRenderPass;
		framebufferCreateInfo.attachmentCount = 1;
		framebufferCreateInfo.pAttachments = &imageView;
		framebufferCreateInfo.width = mSurfaceExtent.width;
		framebufferCreateInfo.height = mSurfaceExtent.height;
		framebufferCreateInfo.layers = 1;

		vkCreateFramebuffer(mLogicalDevice, &framebufferCreateInfo, nullptr, &framebuffer);

		// create fence
		VkFence fence;

		VkFenceCreateInfo fenceCreateInfo = {};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCreateInfo.pNext = nullptr;
		fenceCreateInfo.flags = 0;

		vkCreateFence(mLogicalDevice, &fenceCreateInfo, nullptr, &fence);

		// add frame
		mFrames.push_back({ fence, imageView, image, framebuffer });
	}

	return true;
}

// TODO: make this more versatile and/or move this functionality
VkCommandBuffer GPUEngine::allocateCommandBuffer()
{
	VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

	VkCommandBufferAllocateInfo allocateInfo = {};
	allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocateInfo.pNext = nullptr;
	allocateInfo.commandBufferCount = 1;
	allocateInfo.commandPool = mGraphicsCommandPool;
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	vkAllocateCommandBuffers(mLogicalDevice, &allocateInfo, &commandBuffer);
	return commandBuffer;
}

void GPUEngine::renderFrame()
{
	// Create fence if it does not exist
	// TODO: create sync objects elsewhere
	if (mFence == VK_NULL_HANDLE)
	{
		VkFenceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;

		vkCreateFence(mLogicalDevice, &createInfo, nullptr, &mFence);
	}

	// Acquire image
	uint32_t imageIndex;
	vkAcquireNextImageKHR(mLogicalDevice, mSwapchain, std::numeric_limits<uint64_t>::max(), VK_NULL_HANDLE, mFence, &imageIndex);
	
	// Record command buffer
	VkCommandBuffer commandBuffer = allocateCommandBuffer();

	// begin recording
	{
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.pNext = nullptr;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		beginInfo.pInheritanceInfo = nullptr;
		vkBeginCommandBuffer(commandBuffer, &beginInfo);
	}

	// begin and end render pass
	{
		VkClearValue colorClearValue = { 0.8f, 0.1f, 0.3f, 1.0f };

		VkRenderPassBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		beginInfo.pNext = nullptr;
		beginInfo.renderPass = mRenderPass;
		beginInfo.framebuffer = mFrames[imageIndex].mFramebuffer;
		beginInfo.renderArea.offset = { 0, 0 };
		beginInfo.renderArea.extent = mSurfaceExtent;
		beginInfo.clearValueCount = 1;
		beginInfo.pClearValues = &colorClearValue;
		vkCmdBeginRenderPass(commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdEndRenderPass(commandBuffer);
	}

	vkEndCommandBuffer(commandBuffer);

	// wait for image availability
	vkWaitForFences(mLogicalDevice, 1, &mFence, VK_TRUE, std::numeric_limits<uint64_t>::max());
	vkResetFences(mLogicalDevice, 1, &mFence);

	// submit command buffer
	{
		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = nullptr;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;
		vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, mFence);
	}

	// wait for command buffer completion
	vkWaitForFences(mLogicalDevice, 1, &mFence, VK_TRUE, std::numeric_limits<uint64_t>::max());
	vkResetFences(mLogicalDevice, 1, &mFence);

	// present image
	{
		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.pNext = nullptr;
		presentInfo.waitSemaphoreCount = 0;
		presentInfo.pWaitSemaphores = nullptr;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &mSwapchain;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr;
		vkQueuePresentKHR(mPresentQueue, &presentInfo);
	}

	// free command buffer and reset pool
	vkFreeCommandBuffers(mLogicalDevice, mGraphicsCommandPool, 1, &commandBuffer);
	vkResetCommandPool(mLogicalDevice, mGraphicsCommandPool, 0);
}

bool GPUEngine::chooseSurfaceFormat()
{
	// query available formats
	uint32_t numFormats;
	vkGetPhysicalDeviceSurfaceFormatsKHR(mPhysicalDevice, mSurface, &numFormats, nullptr);
	std::vector<VkSurfaceFormatKHR> formats(numFormats);
	vkGetPhysicalDeviceSurfaceFormatsKHR(mPhysicalDevice, mSurface, &numFormats, formats.data());

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

bool GPUEngine::choosePhysicalDevice(const std::vector<const char*>& extensions)
{
	// enumerate physical devices
	uint32_t physicalDeviceCount = 0;
	vkEnumeratePhysicalDevices(mInstance, &physicalDeviceCount, nullptr);
	std::vector<VkPhysicalDevice> physicalDeviceVector(physicalDeviceCount);
	vkEnumeratePhysicalDevices(mInstance, &physicalDeviceCount, physicalDeviceVector.data());

	// try to find a suitable device
	VkPhysicalDevice bestDevice = VK_NULL_HANDLE;
	bool discreteGPUFound = false;
	for (auto physicalDevice : physicalDeviceVector)
	{
		// Enumerate device queue families
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
		std::vector<VkQueueFamilyProperties> queueFamilyPropertiesVector(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyPropertiesVector.data());

		// Check for graphics and transfer queue
		std::vector<VkQueueFlags> neededQueueFamilies = { VK_QUEUE_GRAPHICS_BIT, VK_QUEUE_TRANSFER_BIT };
		auto acquiredQueues = findDeviceQueueFamilies(physicalDevice, neededQueueFamilies);
		bool allFamiliesValid = true;
		for (auto family : acquiredQueues)
			if (family == INVALID_QUEUE_FAMILY)
			{
				allFamiliesValid = false;
				break;
			}
		if (!allFamiliesValid)
			continue;

		// Check for present queue
		if (findDevicePresentQueueFamily(physicalDevice, mSurface) == INVALID_QUEUE_FAMILY)
			continue;

		// Enumerate device extensions
		uint32_t extensionCount = 0;
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> extensionPropertiesVector(extensionCount);
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, extensionPropertiesVector.data());

		// Check for all required extensions
		std::vector<const char*> extensionCheckVector(extensions);
		for (size_t i=0; i<extensionCheckVector.size(); i++)
		{
			auto requiredName = extensionCheckVector[i];

			for (auto& foundExtension : extensionPropertiesVector)
				if (strcmp(foundExtension.extensionName, requiredName))
				{
					extensionCheckVector.erase(extensionCheckVector.begin() + i);
					i--;
					break;
				}
		}
		if (extensionCheckVector.size() > 0)
			continue;

		// Device meets bare minimum requirements; see if it is more suitable
		// than best previously selected device
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(physicalDevice, &properties);

		if (!discreteGPUFound)
			bestDevice = physicalDevice;
		else if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			bestDevice = physicalDevice;
			discreteGPUFound = true;
		}
	}

	if (bestDevice != VK_NULL_HANDLE)
	{
		mPhysicalDevice = bestDevice;
		return true;
	}

	return false;
}

bool GPUEngine::createLogicalDevice(const std::vector<const char*>& extensions)
{
	// first, grab all required queues
	std::vector<VkQueueFlags> requiredQueues = { VK_QUEUE_GRAPHICS_BIT };
	auto queues = findDeviceQueueFamilies(mPhysicalDevice, requiredQueues);
	for (auto queue : queues)
		if (queue == INVALID_QUEUE_FAMILY)
			return false;

	mGraphicsQueueFamily = queues[0];
	mPresentQueueFamily = findDevicePresentQueueFamily(mPhysicalDevice, mSurface);

	// TODO: make this more robust
	// Create a graphics queue and, if needed, a present queue
	VkDeviceQueueCreateInfo queueCreateInfos[2];
	float queuePriority = 1.0f;

	queueCreateInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfos[0].pNext = nullptr;
	queueCreateInfos[0].pQueuePriorities = &queuePriority;
	queueCreateInfos[0].queueCount = 1;
	queueCreateInfos[0].queueFamilyIndex = mGraphicsQueueFamily;

	queueCreateInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfos[1].pNext = nullptr;
	queueCreateInfos[1].pQueuePriorities = &queuePriority;
	queueCreateInfos[1].queueCount = 1;
	queueCreateInfos[1].queueFamilyIndex = mPresentQueueFamily;

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.queueCreateInfoCount = (mGraphicsQueueFamily == mPresentQueueFamily) ? 1 : 2;
	createInfo.pQueueCreateInfos = queueCreateInfos;
	createInfo.enabledLayerCount = 0;
	createInfo.ppEnabledLayerNames = nullptr;
	fillExtensionsInStruct(createInfo, extensions);
	createInfo.pEnabledFeatures = nullptr;

	VkResult result = vkCreateDevice(mPhysicalDevice, &createInfo, nullptr, &mLogicalDevice);
	if (result != VK_SUCCESS)
		return false;

	// Now obtain and remember the queue(s)
	vkGetDeviceQueue(mLogicalDevice, mGraphicsQueueFamily, 0, &mGraphicsQueue);
	vkGetDeviceQueue(mLogicalDevice, mPresentQueueFamily, 0, &mPresentQueue);

	return true;
}

bool GPUEngine::createCommandPools()
{
	VkCommandPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	createInfo.queueFamilyIndex = mGraphicsQueueFamily;

	VkResult result = vkCreateCommandPool(mLogicalDevice, &createInfo, nullptr, &mGraphicsCommandPool);
	return (result == VK_SUCCESS);
}

bool GPUEngine::createSwapchain()
{
	if (!chooseSurfaceFormat())
		return false;

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.surface = mSurface;
	createInfo.minImageCount = 2;
	createInfo.imageFormat = mSurfaceFormat.format;
	createInfo.imageColorSpace = mSurfaceFormat.colorSpace;
	createInfo.imageExtent = mSurfaceExtent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	uint32_t queueFamilyIndices[] = { mGraphicsQueueFamily, mPresentQueueFamily };
	if (mGraphicsQueueFamily == mPresentQueueFamily)
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
	createInfo.preTransform = VK_SURFACE_TRANSFORM_INHERIT_BIT_KHR;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	VkResult result = vkCreateSwapchainKHR(mLogicalDevice, &createInfo, nullptr, &mSwapchain);
	return (result == VK_SUCCESS);
}

bool GPUEngine::createInstance(const std::vector<const char*>& extensions, std::string appName, std::string engineName, uint32_t appVersion, uint32_t engineVersion)
{
	VkApplicationInfo applicationInfo = {};
	applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	applicationInfo.pNext = nullptr;
	applicationInfo.pApplicationName = appName.c_str();
	applicationInfo.applicationVersion = appVersion;
	applicationInfo.pEngineName = engineName.c_str();
	applicationInfo.engineVersion = engineVersion;
	applicationInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0);

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.pApplicationInfo = &applicationInfo;
	createInfo.enabledLayerCount = 0;
	createInfo.ppEnabledLayerNames = nullptr;
	fillExtensionsInStruct(createInfo, extensions);

	VkResult result = vkCreateInstance(&createInfo, nullptr, &mInstance);
	return (result == VK_SUCCESS);
}

std::vector<const char*> GPUEngine::createInstanceExtensionsVector(const std::vector<GPUProcess*>& processes)
{
	std::vector<const char*> extensionsVector;
	for (auto featureSet : processes)
	{
		uint32_t numExtensions;
		const char** extensions = featureSet->getRequiredInstanceExtensions(&numExtensions);

		for (uint32_t i = 0; i < numExtensions; i++)
		{
			// check if extension is already in the extensions vector
			bool extensionExists = false;
			for (auto extension : extensionsVector)
			{
				if (strcmp(extension, extensions[i]) == 0)
				{
					extensionExists = true;
					break;
				}
			}

			// otherwise, push it back
			if (!extensionExists)
				extensionsVector.push_back(extensions[i]);
		}
	}

	return extensionsVector;
}

std::vector<const char*> GPUEngine::createDeviceExtensionsVector(const std::vector<GPUProcess*>& processes)
{
	std::vector<const char*> extensionsVector;
	for (auto featureSet : processes)
	{
		uint32_t numExtensions;
		const char** extensions = featureSet->getRequiredDeviceExtensions(&numExtensions);

		for (uint32_t i = 0; i < numExtensions; i++)
		{
			// check if extension is already in the extensions vector
			bool extensionExists = false;
			for (auto extension : extensionsVector)
			{
				if (strcmp(extension, extensions[i]) == 0)
				{
					extensionExists = true;
					break;
				}
			}

			// otherwise, push it back
			if (!extensionExists)
				extensionsVector.push_back(extensions[i]);
		}
	}

	return extensionsVector;
}

/**
* Attempts to find queue families matching the requirements given in flags.
* Any families not found will be INVALID_QUEUE_FAMILY in returned vector.
*/
std::vector<uint32_t> GPUEngine::findDeviceQueueFamilies(VkPhysicalDevice device, std::vector<VkQueueFlags>& flags)
{
	// initialize queueFamilies vector
	std::vector<uint32_t> queueFamilies(flags.size());
	for (size_t i = 0; i < queueFamilies.size(); i++)
		queueFamilies[i] = INVALID_QUEUE_FAMILY;

	// query available queue families
	uint32_t familyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount, nullptr);
	std::vector<VkQueueFamilyProperties> propertiesVector(familyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount, propertiesVector.data());

	// find valid families for every set of flags
	size_t familiesNotFound = flags.size();
	for (size_t i = 0; i < familyCount; i++)
	{
		auto& familyProperties = propertiesVector[i];
		for (size_t j = 0; j < queueFamilies.size(); j++)
		{
			// if requested family j has not been found, see if this family
			// satisfies its needs
			if (queueFamilies[j] != INVALID_QUEUE_FAMILY)
				continue;

			if ((familyProperties.queueFlags & flags[j]) == flags[j])
			{
				queueFamilies[j] = i;
				familiesNotFound--;
			}
		}

		// stop looking if all required families have been found
		if (familiesNotFound == 0)
			break;
	}

	return queueFamilies;
}

uint32_t GPUEngine::findDevicePresentQueueFamily(VkPhysicalDevice device, VkSurfaceKHR surface)
{
	uint32_t familyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount, nullptr);

	for (size_t i = 0; i < familyCount; i++)
	{
		VkBool32 supportsPresent;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &supportsPresent);
		if (supportsPresent)
			return i;
	}

	return INVALID_QUEUE_FAMILY;
}