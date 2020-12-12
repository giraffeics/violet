project(vkw_src)

# Main program
add_executable(VKWhatever main.cpp device.cpp)
target_link_libraries(VKWhatever glfw vulkan)