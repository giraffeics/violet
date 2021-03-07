project(violet_shaders)

add_custom_target(violet_shaders)

# add shader sources
set(shaderfiles "phong.vert" "phong.frag")

# find glslc
IF(UNIX)
	set(GLSLC "glslc")
ELSE()
	IF(EXISTS "$ENV{VULKAN_SDK}/Bin32/")
		set(GLSLC "$ENV{VULKAN_SDK}/Bin32/glslc")
	ELSE()
		set(GLSLC "$ENV{VULKAN_SDK}/bin/glslc")
	ENDIF()
ENDIF()

foreach(INFILE ${shaderfiles})
	set(OUTFILE "${INFILE}")
	string(REPLACE ".frag" "_frag.spv" OUTFILE ${OUTFILE})
	string(REPLACE ".vert" "_vert.spv" OUTFILE ${OUTFILE})
	string(REPLACE ".comp" "_comp.spv" OUTFILE ${OUTFILE})
	string(REPLACE ".geom" "_geom.spv" OUTFILE ${OUTFILE})
	message("Shader target added: ${OUTFILE}")
	add_custom_target(
		${OUTFILE}
		COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders
		COMMAND ${GLSLC} ${CMAKE_CURRENT_SOURCE_DIR}/${INFILE} "-o" ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders/${OUTFILE}
		VERBATIM
		SOURCES ${INFILE})
	add_dependencies(violet_shaders ${OUTFILE})
endforeach()