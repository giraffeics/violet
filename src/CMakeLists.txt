project(violet_src)

# Main program
add_executable(violet "main.cpp" "GPUEngine.cpp" "GPUWindowSystemGLFW.cpp" "GPUEngine.h"  "GPUProcess.h" "GPUWindowSystemGLFW.h"  "GPUPipeline.cpp" "GPUProcess.cpp" "GPUProcessRenderPass.h" "GPUProcessRenderPass.cpp" "GPUProcessSwapchain.h" "GPUProcessSwapchain.cpp"  "GPUDependencyGraph.h" "GPUDependencyGraph.cpp"  "GPUMesh.h" "GPUMesh.cpp" "GPUMeshWrangler.cpp" "GPUMeshWrangler.h" "glm_includes.h" "GPUImage.h" "GPUImage.cpp" "GPUPipeline.h")
target_link_libraries(violet glfw vulkan_neat glm assimp)

if ( CMAKE_COMPILER_IS_GNUCC )
    target_compile_options(violet PRIVATE "-Wno-4267" "-Wno-26812")
endif()
if ( MSVC )
    target_compile_options(violet PRIVATE "/wd4267" "/wd26812")
endif()

#Shaders
add_subdirectory(shaders)
add_dependencies(violet violet_shaders)