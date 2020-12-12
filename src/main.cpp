#include <iostream>

#include <GLFW/glfw3.h>

#include "GPUEngine.h"
#include "GPUFeatureSet.h"

class FeatureSetGLFW : public GPUFeatureSet
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

	FeatureSetGLFW featureSet;
	std::vector<GPUFeatureSet*> featureSetVector;
	featureSetVector.push_back(&featureSet);

	GPUEngine engine(featureSetVector, "VKWhatever", "Whatever Engine");

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}