project(vkw_src)

# Main program
add_executable(VKWhatever "main.cpp" "GPUEngine.cpp" "GPUWindowSystemGLFW.cpp" "GPUEngine.h"  "GPUProcess.h" "GPUWindowSystemGLFW.h"  "GPUPipeline.cpp" "GPUProcess.cpp" "GPUProcessRenderPass.h" "GPUProcessRenderPass.cpp" "GPUProcessSwapchain.h" "GPUProcessSwapchain.cpp")
target_link_libraries(VKWhatever glfw vulkan_neat)

#Shaders
add_subdirectory(shaders)
add_dependencies(VKWhatever vkw_shaders)