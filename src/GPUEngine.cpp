#include "GPUEngine.h"

#include <iostream>

GPUEngine::GPUEngine(const std::vector<GPUFeatureSet*>& featureSets, std::string appName, std::string engineName, uint32_t appVersion, uint32_t engineVersion)
{
	auto extensions = createExtensionsVector(featureSets);

	if(createInstance(extensions, appName, engineName, appVersion, engineVersion))
		std::cout << "Instance created successfully!!" << std::endl;
	else
		std::cout << "Could not create instance!!" << std::endl;

	if (choosePhysicalDevice(extensions))
		std::cout << "Physical device selected successfully!!" << std::endl;
	else
		std::cout << "Could not find a suitable physical device!!" << std::endl;
}

bool GPUEngine::choosePhysicalDevice(const std::vector<const char*> extensions)
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

bool GPUEngine::createInstance(const std::vector<const char*> extensions, std::string appName, std::string engineName, uint32_t appVersion, uint32_t engineVersion)
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
	createInfo.enabledExtensionCount = extensions.size();
	createInfo.ppEnabledExtensionNames = extensions.data();

	VkResult result = vkCreateInstance(&createInfo, nullptr, &mInstance);
	return (result == VK_SUCCESS);
}

std::vector<const char*> GPUEngine::createExtensionsVector(const std::vector<GPUFeatureSet*>& featureSets)
{
	std::vector<const char*> extensionsVector;
	for (auto featureSet : featureSets)
	{
		uint32_t numExtensions;
		const char** extensions = featureSet->getRequiredExtensions(&numExtensions);

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