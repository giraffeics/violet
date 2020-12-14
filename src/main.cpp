#include <iostream>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include "GPUEngine.h"
#include "GPUProcess.h"

class ProcessGLFW : public GPUWindowSystem
{
public:
	ProcessGLFW()
	{
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		mWindow = glfwCreateWindow(640, 480, "Hello, World~!! ^-^", nullptr, nullptr);
	}

	~ProcessGLFW()
	{
		glfwDestroyWindow(mWindow);
		glfwTerminate();
	}

	virtual const char** getRequiredInstanceExtensions(uint32_t* count)
	{
		return glfwGetRequiredInstanceExtensions(count);
	}

	virtual const char** getRequiredDeviceExtensions(uint32_t* count)
	{
		*count = 1;
		return &swapchainExtensionName;
	}

	virtual VkSurfaceKHR createSurface(VkInstance instance)
	{
		VkSurfaceKHR surface;
		VkResult result = glfwCreateWindowSurface(instance, mWindow, nullptr, &surface);
		if (result != VK_SUCCESS)
			return VK_NULL_HANDLE;
		return surface;
	}

	virtual VkExtent2D getSurfaceExtent()
	{
		int width, height;
		glfwGetWindowSize(mWindow, &width, &height);
		return VkExtent2D{ (uint32_t)width, (uint32_t)height };
	}

	void pollEvents()
	{
		glfwPollEvents();
	}

	bool shouldClose()
	{
		return glfwWindowShouldClose(mWindow);
	}

private:
	GLFWwindow* mWindow = nullptr;
	const char* swapchainExtensionName = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
};

int main()
{
	ProcessGLFW process;
	std::vector<GPUProcess*> processVector;
	processVector.push_back(&process);

	GPUEngine engine(processVector, &process, "VKWhatever", "Whatever Engine");

	while (!process.shouldClose())
	{
		process.pollEvents();
	}

	return 0;
}