#include <iostream>
#include <GLFW/glfw3.h>

#include "device.h"

int main()
{
	glfwInit();

	GLFWwindow* window = glfwCreateWindow(640, 480, "Hello, World~!! ^-^", nullptr, nullptr);

	device d;
	d.sayHello();

	glfwShowWindow(window);

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}