cmake_minimum_required(VERSION 3.8)

project("VKWhatever")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# ADD PROJECT SOURCES
add_subdirectory("src")

# SET UP VULKAN LIBRARY

find_package(Vulkan REQUIRED FATAL_ERROR)

add_library(vulkan_neat INTERFACE)
target_link_libraries(vulkan_neat INTERFACE ${Vulkan_LIBRARIES})
target_include_directories(vulkan_neat INTERFACE ${Vulkan_INCLUDE_DIRS})

# SET UP GLFW LIBRARY
option(BUILD_SHARED_LIBS off)
add_subdirectory("glfw")
include_directories("glfw/include")

# SET UP GLM LIBRARY
add_subdirectory("glm")
include_directories("glm")

# SET UP ASSIMP LIBRARY
add_subdirectory("assimp")
include_directories("assimp/include")
