project(vkw_src)

# Main program
add_executable(VKWhatever "main.cpp" "GPUEngine.cpp" "GPUEngine.h"  "GPUFeatureSet.h")
target_link_libraries(VKWhatever glfw vulkan)