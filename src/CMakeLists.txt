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
add_executable(violet "main.cpp")
list(TRANSFORM violet_sources PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")
list(TRANSFORM violet_headers PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")
target_sources(violet PUBLIC ${violet_headers})
target_sources(violet PRIVATE ${violet_sources})
target_link_libraries(violet PRIVATE glfw vulkan_neat glm assimp)

# suppress compiler warnings related to vulkan headers
if ( CMAKE_COMPILER_IS_GNUCC )
    target_compile_options(violet PRIVATE "-Wno-4267" "-Wno-26812")
endif()
if ( MSVC )
    target_compile_options(violet PRIVATE "/wd4267" "/wd26812")
endif()

# compile shaders
add_subdirectory(shaders)
add_dependencies(violet violet_shaders)