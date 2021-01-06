project(vkw_src)

# Main program
add_executable(VKWhatever "main.cpp" "GPUEngine.cpp" "GPUWindowSystemGLFW.cpp" "GPUEngine.h"  "GPUProcess.h" "GPUWindowSystemGLFW.h"  "GPUPipeline.cpp" "GPUProcess.cpp" "GPUProcessRenderPass.h" "GPUProcessRenderPass.cpp" "GPUProcessSwapchain.h" "GPUProcessSwapchain.cpp"  "GPUDependencyGraph.h" "GPUDependencyGraph.cpp"  "GPUMesh.h" "GPUMesh.cpp")
target_link_libraries(VKWhatever glfw vulkan_neat glm tiny_gltf)

#Shaders
add_subdirectory(shaders)
add_dependencies(VKWhatever vkw_shaders)