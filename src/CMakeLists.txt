project(vkw_src)

# Main program
add_executable(VKWhatever "main.cpp" "GPUEngine.cpp" "GPUWindowSystemGLFW.cpp" "GPUEngine.h"  "GPUProcess.h" "GPUWindowSystemGLFW.h")
target_link_libraries(VKWhatever glfw vulkan_neat)