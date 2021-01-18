#include <iostream>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include "GPUEngine.h"
#include "GPUMeshWrangler.h"
#include "GPUWindowSystemGLFW.h"

int main()
{
	GPUWindowSystemGLFW windowSystem;
	std::vector<GPUProcess*> processVector;
	processVector.push_back(&windowSystem);

	GPUEngine engine(processVector, &windowSystem, "VKWhatever", "Whatever Engine");
	auto meshWrangler = engine.getMeshWrangler();

	GPUMesh faceMesh("face.glb", &engine);
	faceMesh.load();
	GPUMesh::Instance meshInstance1;
	meshInstance1.mMesh = &faceMesh;
	glm::vec3 translation1 = { -1.0, -1.0, 0.0 };
	glm::vec3 axis1 = { 0.0, 0.0, 1.0 };

	GPUMesh::Instance meshInstance2;
	meshInstance2.mMesh = &faceMesh;
	glm::vec3 translation2 = { 1.0, 1.5, -1.0 };
	glm::vec3 axis2 = { 1.0, 1.0, 0.0 };
	axis2 = glm::normalize(axis2);
	
	float rot = 0.0;

	while (!windowSystem.shouldClose())
	{
		meshWrangler->reset();
		meshInstance1.mTransform = glm::translate(translation1) * glm::rotate(rot, axis1);
		meshInstance2.mTransform = glm::translate(translation2) * glm::rotate(rot, axis2);
		meshWrangler->stageMeshInstance(&meshInstance1);
		meshWrangler->stageMeshInstance(&meshInstance2);
		windowSystem.pollEvents();
		engine.renderFrame();

		rot += 0.01;
	}

	return 0;
}