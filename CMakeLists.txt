cmake_minimum_required(VERSION 3.8)

project("VKWhatever")

# ADD PROJECT SOURCES
add_executable("${PROJECT_NAME}" "src/main.cpp")

file(GLOB_RECURSE SOURCES "src/*.cpp")
list(REMOVE_ITEM SOURCES "src/main.cpp")
target_sources("${PROJECT_NAME}" PRIVATE "${SOURCES}")

include_directories(src/include)

set_property(TARGET "${PROJECT_NAME}" PROPERTY CXX_STANDARD 17)
set_property(TARGET "${PROJECT_NAME}" PROPERTY C_STANDARD 17)

# SET UP VULKAN LIBRARY
include_directories("$ENV{VULKAN_SDK}/Include")

set(VKLIBPATH "$ENV{VULKAN_SDK}/Lib32")

if (${CMAKE_SIZEOF_VOID_P} MATCHES 8)
	set(VKLIBPATH "$ENV{VULKAN_SDK}/Lib")
endif()

target_link_libraries("${PROJECT_NAME}" "${VKLIBPATH}/vulkan-1.lib")

# SET UP GLFW LIBRARY
option(BUILD_SHARED_LIBS ON)
add_subdirectory("glfw")
include_directories("glfw/include")
target_link_libraries("${PROJECT_NAME}" glfw ${GLFW_LIBRARIES})

# SET UP GLM LIBRARY
add_subdirectory("glm")
include_directories("glm")
target_link_libraries("${PROJECT_NAME}" glm)

# SET UP SHADER COMPILATION
set(GLSLC "$ENV{VULKAN_SDK}/Bin32/glslc")

if(WIN32)
	set(GLSLC "$ENV{VULKAN_SDK}/Bin32/glslc.exe")
endif()

add_custom_target(shaders)

file(GLOB shaderfiles "src/shaders/*.*")
foreach(SHADER ${shaderfiles})
	string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/" "${CMAKE_CURRENT_BINARY_DIR}/shaders/" OUTFILE ${SHADER})
	string(REPLACE ".frag" "_frag.spv" OUTFILE ${OUTFILE})
	string(REPLACE ".vert" "_vert.spv" OUTFILE ${OUTFILE})
	string(REPLACE ".comp" "_comp.spv" OUTFILE ${OUTFILE})
	string(REPLACE ".geom" "_geom.spv" OUTFILE ${OUTFILE})
	get_filename_component(OUTDIR ${OUTFILE} DIRECTORY)
	message("Shader target added: ${OUTFILE}")
	add_custom_command( TARGET shaders
						COMMAND ${CMAKE_COMMAND} -E make_directory "${OUTDIR}"
						COMMAND ${GLSLC} ${SHADER} "-o" ${OUTFILE}
						VERBATIM
						MAIN_DEPENDENCY "${SHADER}") 
endforeach()

add_dependencies("${PROJECT_NAME}" shaders)