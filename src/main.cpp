#include <iostream>

#include <GLFW/glfw3.h>

#include "GPUEngine.h"
#include "GPUProcess.h"

class ProcessGLFW : public GPUProcess
{
public:
	virtual const char** getRequiredInstanceExtensions(uint32_t* count)
	{
		return glfwGetRequiredInstanceExtensions(count);
	}
};

int main()
{
	glfwInit();

	GLFWwindow* window = glfwCreateWindow(640, 480, "Hello, World~!! ^-^", nullptr, nullptr);

	ProcessGLFW process;
	std::vector<GPUProcess*> processVector;
	processVector.push_back(&process);

	GPUEngine engine(processVector, "VKWhatever", "Whatever Engine");

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}