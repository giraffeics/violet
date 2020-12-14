#include <iostream>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include "GPUEngine.h"
#include "GPUProcess.h"
#include "GPUWindowSystemGLFW.h"

int main()
{
	GPUWindowSystemGLFW windowSystem;
	std::vector<GPUProcess*> processVector;
	processVector.push_back(&windowSystem);

	GPUEngine engine(processVector, &windowSystem, "VKWhatever", "Whatever Engine");

	while (!windowSystem.shouldClose())
	{
		windowSystem.pollEvents();
	}

	return 0;
}