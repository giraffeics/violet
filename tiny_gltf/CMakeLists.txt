project(tiny_gltf)

add_library(tiny_gltf STATIC "tiny_gltf_implementation.cpp" "json.hpp" "stb_image.h" "stb_image_write.h" "tiny_gltf.h")
target_include_directories(tiny_gltf INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
