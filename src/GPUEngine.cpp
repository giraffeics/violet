#include "GPUEngine.h"

#include <iostream>
#include <limits>

GPUEngine::GPUEngine(const std::vector<GPUProcess*>& processes, std::string appName, std::string engineName, uint32_t appVersion, uint32_t engineVersion)
{
	auto instanceExtensions = createInstanceExtensionsVector(processes);
	auto deviceExtensions = createDeviceExtensionsVector(processes);

	if(createInstance(instanceExtensions, appName, engineName, appVersion, engineVersion))
		std::cout << "Instance created successfully!!" << std::endl;
	else
		std::cout << "Could not create instance!!" << std::endl;

	if (choosePhysicalDevice(deviceExtensions))
		std::cout << "Physical device selected successfully!!" << std::endl;
	else
		std::cout << "Could not find a suitable physical device!!" << std::endl;

	if (createLogicalDevice(deviceExtensions))
		std::cout << "Locigal device created successfully!!" << std::endl;
	else
		std::cout << "Could not create logical device!!" << std::endl;
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
		bool graphicsQueueFound = false;
		bool transferQueueFound = false;
		for (auto& queueFamilyProperties : queueFamilyPropertiesVector)
		{
			if (queueFamilyProperties.queueFlags | VK_QUEUE_GRAPHICS_BIT)
				graphicsQueueFound = true;
			if (queueFamilyProperties.queueFlags | VK_QUEUE_TRANSFER_BIT)
				transferQueueFound = true;
		}
		if (!(graphicsQueueFound && transferQueueFound))
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

	// TODO: make this more robust
	// Create a single, graphics queue
	float queuePriority = 1.0f;
	VkDeviceQueueCreateInfo queueCreateInfo = {};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.pNext = nullptr;
	queueCreateInfo.pQueuePriorities = &queuePriority;
	queueCreateInfo.queueCount = 1;
	queueCreateInfo.queueFamilyIndex = mGraphicsQueueFamily;

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.queueCreateInfoCount = 1;
	createInfo.pQueueCreateInfos = &queueCreateInfo;
	createInfo.enabledLayerCount = 0;
	createInfo.ppEnabledLayerNames = nullptr;
	fillExtensionsInStruct(createInfo, extensions);
	createInfo.pEnabledFeatures = nullptr;

	VkResult result = vkCreateDevice(mPhysicalDevice, &createInfo, nullptr, &mLogicalDevice);
	if (result != VK_SUCCESS)
		return false;

	// Now obtain and remember the graphics queue
	vkGetDeviceQueue(mLogicalDevice, mGraphicsQueueFamily, 0, &mGraphicsQueue);

	return true;
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
