cmake_minimum_required(VERSION 3.8)

project("violet_thirdparty")

# SET UP GLFW LIBRARY
set(BUILD_SHARED_LIBS off)
add_subdirectory("glfw")
include_directories("glfw/include")

# SET UP GLM LIBRARY
add_subdirectory("glm")
include_directories("glm")

# SET UP ASSIMP LIBRARY
set(ASSIMP_BUILD_TESTS OFF)
add_subdirectory("assimp")
include_directories("assimp/include")
