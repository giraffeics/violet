#ifndef GPUWINDOWSYSTEMGLFW_H
#define GPUWINDOWSYSTEMGLFW_H

#include "GPUProcess.h"
#include <GLFW/glfw3.h>

/**
 * @brief A GPUWindowSystem that uses the GLFW cross-platform library.
 * 
 * This is essentially an abstraction over an abstraction, but it's useful because it
 * allows OOP to be used even though GLFW is a C library, and it will make it easier
 * to support platforms that GLFW has not been ported to.
 */
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