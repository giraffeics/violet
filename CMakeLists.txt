cmake_minimum_required(VERSION 3.12)

project("violet")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# ADD PROJECT SOURCES
add_subdirectory("src")

# SET UP VULKAN LIBRARY

find_package(Vulkan REQUIRED FATAL_ERROR)

add_library(vulkan_neat INTERFACE)
target_link_libraries(vulkan_neat INTERFACE ${Vulkan_LIBRARIES})
target_include_directories(vulkan_neat INTERFACE ${Vulkan_INCLUDE_DIRS})

# SET UP THIRD-PARTY SOURCE LIBRARIES
add_subdirectory("thirdparty")