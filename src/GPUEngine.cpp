#include "GPUEngine.h"

#include <iostream>
#include <limits>
#include <cstring>

#include "GPUProcessRenderPass.h"
#include "GPUProcessSwapchain.h"
#include "GPUImage.h"

#ifdef NDEBUG
	std::vector<const char*> GPUEngine::validationLayers;
#else
	std::vector<const char*> GPUEngine::validationLayers = { "VK_LAYER_KHRONOS_validation" };
#endif

VkBool32 GPUEngine::vulkanDebugCallback( VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
							const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData)
{
	if(messageSeverity != VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
		std::cerr << "Vulkan Validation: " << pCallbackData->pMessage << std::endl;
	return VK_FALSE;
}

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

	#ifndef NDEBUG
		instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	#endif

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

	if (createDescriptorSetLayout())
		std::cout << "Descriptor set layout created successfully!!" << std::endl;
	else
		std::cout << "Could not create descriptor set layout!!" << std::endl;

	createDebugMessenger();

	// create transfer fence
	mTransferFence = createFence(0);

	// create dependency graph
	mDependencyGraph = std::make_unique<GPUDependencyGraph>(this);

	// create mesh wrangler
	mMeshWrangler = new GPUMeshWrangler;
	addProcess(mMeshWrangler);

	// create swapchain
	mSwapchainProcess = new GPUProcessSwapchain;
	addProcess(mSwapchainProcess);
	addProcess(mSwapchainProcess->getPresentProcess());
}

GPUEngine::~GPUEngine()
{
	// explicitly delete unique pointers owning vulkan handles
	// (destructor must be called while instance exists)
	mDependencyGraph.reset();

	// destroy command pools
	vkDestroyCommandPool(mDevice, mGraphicsCommandPool, nullptr);

	// destroy other owned objects
	if(mDebugMessenger != VK_NULL_HANDLE)
	{
		auto vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(mInstance, "vkDestroyDebugUtilsMessengerEXT");
		vkDestroyDebugUtilsMessengerEXT(mInstance, mDebugMessenger, nullptr);
	}
	vkDestroyFence(mDevice, mTransferFence, nullptr);
	vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
	vkDestroyDescriptorSetLayout(mDevice, mDescriptorLayoutModel, nullptr);

	// finally, destroy device and instance
	vkDestroyDevice(mDevice, nullptr);
	vkDestroyInstance(mInstance, nullptr);
}

bool GPUEngine::createSurface()
{
	mSurface = mWindowSystem->createSurface(mInstance);
	if (mSurface == VK_NULL_HANDLE)
		return false;

	mSurfaceExtent = mWindowSystem->getSurfaceExtent();
	return true;
}

// TODO: make this more versatile and/or move this functionality
VkCommandBuffer GPUEngine::allocateCommandBuffer(VkCommandPool commandPool)
{
	VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

	VkCommandBufferAllocateInfo allocateInfo = {};
	allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocateInfo.pNext = nullptr;
	allocateInfo.commandBufferCount = 1;
	allocateInfo.commandPool = commandPool;
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	vkAllocateCommandBuffers(mDevice, &allocateInfo, &commandBuffer);
	return commandBuffer;
}

VkSemaphore GPUEngine::createSemaphore()
{
	VkSemaphore semaphore;
	VkSemaphoreCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	vkCreateSemaphore(mDevice, &createInfo, nullptr, &semaphore);
	return semaphore;
}

VkFence GPUEngine::createFence(VkFenceCreateFlags flags)
{
	VkFence fence;

	VkFenceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.flags = flags;

	vkCreateFence(mDevice, &createInfo, nullptr, &fence);
	return fence;
}

bool GPUEngine::createBuffer(VkDeviceSize size, VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryFlags, VkBuffer& buffer, VkDeviceMemory& memory)
{
	// create the buffer
	VkBufferCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.size = size;
	createInfo.usage = usageFlags;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.queueFamilyIndexCount = 0;

	if (vkCreateBuffer(mDevice, &createInfo, nullptr, &buffer) != VK_SUCCESS)
		return false;

	// find a suitable memory type
	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(mDevice, buffer, &memoryRequirements);
	uint32_t memoryType = findMemoryType(memoryRequirements.memoryTypeBits, memoryFlags);
	if (memoryType == UINT32_MAX)
		return false;

	// allocate memory
	VkMemoryAllocateInfo allocateInfo = {};
	allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocateInfo.pNext = nullptr;
	allocateInfo.allocationSize = memoryRequirements.size;
	allocateInfo.memoryTypeIndex = memoryType;

	if (vkAllocateMemory(mDevice, &allocateInfo, nullptr, &memory) != VK_SUCCESS)
		return false;

	// bind memory
	if (vkBindBufferMemory(mDevice, buffer, memory, 0) != VK_SUCCESS)
		return false;

	return true;
}

// TODO: optimize this to avoid unnecessary memory allocation and play nice
// with multiple threads
void GPUEngine::transferToBuffer(VkBuffer destination, void* data, VkDeviceSize size, VkDeviceSize offset)
{
	// create staging buffer
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingMemory;
	createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, stagingBuffer, stagingMemory);

	// stage & transfer data
	// TODO: optimize staging & transfer to avoid unnecessary memory allocation
	void* dataPtr;
	vkMapMemory(mDevice, stagingMemory, 0, size, 0, &dataPtr);
		memcpy(dataPtr, data, (size_t)size);
	vkUnmapMemory(mDevice, stagingMemory);

	{
		VkCommandBuffer commandBuffer = allocateCommandBuffer(mGraphicsCommandPool);
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.pNext = nullptr;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		VkBufferCopy copyRegion = {};
		copyRegion.srcOffset = 0;
		copyRegion.dstOffset = 0;
		copyRegion.size = size;
		vkCmdCopyBuffer(commandBuffer, stagingBuffer, destination, 1, &copyRegion);

		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pNext = nullptr;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, mTransferFence);
		vkWaitForFences(mDevice, 1, &mTransferFence, VK_TRUE, UINT64_MAX);
		vkResetFences(mDevice, 1, &mTransferFence);

		vkFreeCommandBuffers(mDevice, mGraphicsCommandPool, 1, &commandBuffer);
	}

	vkFreeMemory(mDevice, stagingMemory, nullptr);
	vkDestroyBuffer(mDevice, stagingBuffer, nullptr);
}

uint32_t GPUEngine::findMemoryType(uint32_t memoryTypeBits, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(mPhysicalDevice, &memoryProperties);

	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
		if ((memoryTypeBits & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & properties))
		{
			return i;
		}

	return UINT32_MAX;
}

void GPUEngine::addProcess(GPUProcess* process)
{
	process->setEngine(this);
	mDependencyGraph->addProcess(process);
}

void GPUEngine::validateProcesses()
{
	mDependencyGraph->build();
}

void GPUEngine::renderFrame()
{
	mDependencyGraph->executeSequence();

	if (((GPUProcessSwapchain*)mSwapchainProcess)->shouldRebuild())
	{
		mDependencyGraph->invalidateFrameResources();
		vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
		createSurface();
		// TODO: query this in a better way
		findDevicePresentQueueFamily(mPhysicalDevice, mSurface);
		mDependencyGraph->acquireFrameResources();
	}
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
		{
			bestDevice = physicalDevice;
			mPhysicalDeviceLimits.reset(new VkPhysicalDeviceLimits(properties.limits));
		}
		if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			bestDevice = physicalDevice;
			mPhysicalDeviceLimits.reset(new VkPhysicalDeviceLimits(properties.limits));
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
	queueCreateInfos[0].flags = 0;
	queueCreateInfos[0].pQueuePriorities = &queuePriority;
	queueCreateInfos[0].queueCount = 1;
	queueCreateInfos[0].queueFamilyIndex = mGraphicsQueueFamily;

	queueCreateInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfos[1].pNext = nullptr;
	queueCreateInfos[1].flags = 0;
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

	VkResult result = vkCreateDevice(mPhysicalDevice, &createInfo, nullptr, &mDevice);
	if (result != VK_SUCCESS)
		return false;

	// Now obtain and remember the queue(s)
	vkGetDeviceQueue(mDevice, mGraphicsQueueFamily, 0, &mGraphicsQueue);
	vkGetDeviceQueue(mDevice, mPresentQueueFamily, 0, &mPresentQueue);

	return true;
}

bool GPUEngine::createCommandPools()
{
	VkCommandPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	createInfo.queueFamilyIndex = mGraphicsQueueFamily;

	VkResult result = vkCreateCommandPool(mDevice, &createInfo, nullptr, &mGraphicsCommandPool);
	return (result == VK_SUCCESS);
}

bool GPUEngine::createDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding binding = {};
	binding.binding = 0;
	binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	binding.descriptorCount = 1;
	binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	binding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.bindingCount = 1;
	createInfo.pBindings = &binding;

	return (vkCreateDescriptorSetLayout(mDevice, &createInfo, nullptr, &mDescriptorLayoutModel) == VK_SUCCESS);
}

bool GPUEngine::createDebugMessenger()
{
	#ifdef NDEBUG
		return true;
	#else
		auto vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(mInstance, "vkCreateDebugUtilsMessengerEXT");

		VkDebugUtilsMessengerCreateInfoEXT createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.pNext = nullptr;
		createInfo.flags = 0;
		createInfo.messageSeverity = 	VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
										VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
										VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
										VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType =	VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
									VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
									VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = [](VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
							const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData )-> VkBool32 {
								return ((GPUEngine*)pUserData)->vulkanDebugCallback(messageSeverity, messageType, pCallbackData);
							};

		VkResult result = vkCreateDebugUtilsMessengerEXT(mInstance, &createInfo, nullptr, &mDebugMessenger);
		return (result == VK_TRUE);
	#endif
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
	createInfo.enabledLayerCount = validationLayers.size();
	createInfo.ppEnabledLayerNames = validationLayers.data();
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
