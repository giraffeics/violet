cmake_minimum_required(VERSION 3.8)

project("VKWhatever")

# ADD PROJECT SOURCES
add_subdirectory("src")

# SET UP VULKAN LIBRARY
add_library(vulkan INTERFACE)
target_include_directories(vulkan INTERFACE "$ENV{VULKAN_SDK}/Include")

set(VKLIBPATH "$ENV{VULKAN_SDK}/Lib32")

if (${CMAKE_SIZEOF_VOID_P} MATCHES 8)
	set(VKLIBPATH "$ENV{VULKAN_SDK}/Lib")
endif()

target_link_libraries(vulkan INTERFACE "${VKLIBPATH}/vulkan-1.lib")

# SET UP GLFW LIBRARY
option(BUILD_SHARED_LIBS ON)
add_subdirectory("glfw")
include_directories("glfw/include")

# SET UP GLM LIBRARY
add_subdirectory("glm")
include_directories("glm")
