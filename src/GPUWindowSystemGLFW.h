#ifndef GPUWINDOWSYSTEMGLFW_H
#define GPUWINDOWSYSTEMGLFW_H

#include "GPUProcess.h"
#include <GLFW/glfw3.h>

class GPUWindowSystemGLFW : public GPUWindowSystem
{
public:
	GPUWindowSystemGLFW();
	GPUWindowSystemGLFW(GPUWindowSystemGLFW& other) = delete;
	GPUWindowSystemGLFW(GPUWindowSystemGLFW&& other) = delete;
	GPUWindowSystemGLFW& operator=(GPUWindowSystemGLFW& other) = delete;
	~GPUWindowSystemGLFW();

	virtual const char** getRequiredInstanceExtensions(uint32_t* count);

	virtual const char** getRequiredDeviceExtensions(uint32_t* count);

	virtual VkSurfaceKHR createSurface(VkInstance instance);

	virtual VkExtent2D getSurfaceExtent();

	void pollEvents();

	bool shouldClose();

private:
	GLFWwindow* mWindow = nullptr;
	const char* swapchainExtensionName = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
};

#endif