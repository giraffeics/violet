cmake_minimum_required(VERSION 3.12)

project(violet_src)

# list violet executable sources
list(APPEND violet_sources
    "GPUEngine.cpp"
    "GPUProcess.cpp"
    "GPUDependencyGraph.cpp"
    "GPUPipeline.cpp"
    "GPUProcessRenderPass.cpp"
    "GPUProcessSwapchain.cpp"
    "GPUMesh.cpp"
    "GPUMeshWrangler.cpp"
    "GPUImage.cpp"
    "GPUWindowSystemGLFW.cpp"
)

# list violet executable headers
list(APPEND violet_headers
    "GPUEngine.h"
    "GPUProcess.h"
    "GPUDependencyGraph.h"
    "GPUPipeline.h"
    "GPUProcessRenderPass.h"
    "GPUProcessSwapchain.h"
    "GPUMesh.h"
    "GPUMeshWrangler.h"
    "GPUImage.h"
    "GPUWindowSystemGLFW.h"
    "glm_includes.h"
)

# configure violet executable target
add_executable(violet "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")
list(TRANSFORM violet_sources PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")
list(TRANSFORM violet_headers PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")
target_sources(violet PUBLIC ${violet_headers})
target_sources(violet PRIVATE ${violet_sources})
target_link_libraries(violet PRIVATE glfw vulkan_neat glm assimp)

# compile shaders
add_subdirectory(shaders)
add_dependencies(violet violet_shaders)