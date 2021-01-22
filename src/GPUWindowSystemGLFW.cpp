#include "GPUWindowSystemGLFW.h"

#include <iostream>

GPUWindowSystemGLFW::GPUWindowSystemGLFW()
{
	int result = glfwInit();
	if(!result)
	{
		const char* description;
		result = glfwGetError((const char**) &description);
		std::cout << "ERROR #" << result << std::endl;
		if(description)
			std::cout << description << std::endl;
	}
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	mWindow = glfwCreateWindow(640, 480, "Hello, World~!! ^-^", nullptr, nullptr);
}

GPUWindowSystemGLFW::~GPUWindowSystemGLFW()
{
	glfwDestroyWindow(mWindow);
	glfwTerminate();
}

const char** GPUWindowSystemGLFW::getRequiredInstanceExtensions(uint32_t* count)
{
	return glfwGetRequiredInstanceExtensions(count);
}

const char** GPUWindowSystemGLFW::getRequiredDeviceExtensions(uint32_t* count)
{
	*count = 1;
	return &swapchainExtensionName;
}

VkSurfaceKHR GPUWindowSystemGLFW::createSurface(VkInstance instance)
{
	VkSurfaceKHR surface;
	VkResult result = glfwCreateWindowSurface(instance, mWindow, nullptr, &surface);
	if (result != VK_SUCCESS)
		return VK_NULL_HANDLE;
	return surface;
}

VkExtent2D GPUWindowSystemGLFW::getSurfaceExtent()
{
	int width, height;
	glfwGetWindowSize(mWindow, &width, &height);
	return VkExtent2D{ (uint32_t)width, (uint32_t)height };
}

void GPUWindowSystemGLFW::pollEvents()
{
	glfwPollEvents();
}

bool GPUWindowSystemGLFW::shouldClose()
{
	return glfwWindowShouldClose(mWindow);
}