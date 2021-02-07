#include <iostream>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include "GPUEngine.h"
#include "GPUMeshWrangler.h"
#include "GPUImage.h"
#include "GPUProcessRenderPass.h"
#include "GPUProcessSwapchain.h"
#include "GPUWindowSystemGLFW.h"

/**
 * @brief Current entry point for the application.
 * 
 * Tests rendering functionality by drawing some rotating meshes.
 * Will later be replaced as the project shifts towards a more general-purpose usable state.
 * 
 * @return int 
 */
int main()
{
	// create window system
	GPUWindowSystemGLFW windowSystem;
	std::vector<GPUProcess*> processVector;
	processVector.push_back(&windowSystem);

	// create GPUEngine and obtain mesh wrangler
	GPUEngine engine(processVector, &windowSystem, "Violet Test", "Violet Engine");
	auto meshWrangler = engine.getMeshWrangler();

	// set up GPUProcesses and passable resource relationships
	{
		// obtain swapchain and present processes
		auto swapchainProcess = engine.getSwapchainProcess();
		auto presentProcess = engine.getPresentProcess();

		// create a process to manage the Z/depth buffer image
		auto zBufferImage = new GPUImage(VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_TILING_OPTIMAL, 1);

		// create a render pass which renders color to the swapchain's image,
		// uses zBufferImage as its depth buffer, and reads from the mesh wrangler's uniform buffer
		auto renderPassProcess = new GPUProcessRenderPass;
		renderPassProcess->setImageViewPR(swapchainProcess->getPRImageView());
		renderPassProcess->setZBufferViewPR(zBufferImage->getImageViewPR());
		renderPassProcess->setUniformBufferPR(meshWrangler->getPRUniformBuffer());

		// tell the present process to present after the render pass is done rendering
		presentProcess->setImageViewInPR(renderPassProcess->getImageViewOutPR());

		// add the Z-buffer image, and the render pass, to the engine's dependency graph
		engine.addProcess(zBufferImage);
		engine.addProcess(renderPassProcess);

		// build the dependency graph
		engine.validateProcesses();
	}

	// load the 3D mesh contained in assets/face.glb
	GPUMesh faceMesh("face.obj", &engine);
	faceMesh.load();

	// create two mesh instances and associated transformation data
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
		// wait for input
		windowSystem.pollEvents();

		// reset the mesh wrangler so mesh instances can be (re-)staged
		meshWrangler->reset();

		// update the transformation data of the mesh instances
		meshInstance1.mTransform = glm::translate(translation1) * glm::rotate(rot, axis1);
		meshInstance2.mTransform = glm::translate(translation2) * glm::rotate(rot, axis2);

		// stage the mesh instances
		meshWrangler->stageMeshInstance(&meshInstance1);
		meshWrangler->stageMeshInstance(&meshInstance2);

		// render and present the frame
		engine.renderFrame();

		// increment rotation angle for the next frame
		rot += 0.01;
	}

	return 0;
}