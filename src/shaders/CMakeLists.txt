project(vkw_shaders)

add_custom_target(vkw_shaders)

# add shader sources
set(shaderfiles "passthrough.vert" "passthrough.frag")

# find glslc
IF(EXISTS "$ENV{VULKAN_SDK}/Bin32/")
	set(GLSLC "$ENV{VULKAN_SDK}/Bin32/glslc")
ELSE()
	set(GLSLC "$ENV{VULKAN_SDK}/bin/glslc")
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
		COMMAND ${GLSLC} ${CMAKE_CURRENT_SOURCE_DIR}/${INFILE} "-o" ${CMAKE_CURRENT_BINARY_DIR}/${OUTFILE}
		VERBATIM
		SOURCES ${INFILE})
	add_dependencies(vkw_shaders ${OUTFILE})
endforeach()

# compile shaders
#foreach(SHADER ${shaderfiles})
#	#string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}" OUTFILE ${SHADER})
#	set(INFILE "${CMAKE_CURRENT_SOURCE_DIR}/${SHADER}")
#	set(OUTFILE "${CMAKE_CURRENT_BINARY_DIR}/${SHADER}")
#	string(REPLACE ".frag" "_frag.spv" OUTFILE ${OUTFILE})
#	string(REPLACE ".vert" "_vert.spv" OUTFILE ${OUTFILE})
#	string(REPLACE ".comp" "_comp.spv" OUTFILE ${OUTFILE})
#	string(REPLACE ".geom" "_geom.spv" OUTFILE ${OUTFILE})
#	get_filename_component(OUTDIR ${OUTFILE} DIRECTORY)
#	message("Shader target added: ${OUTFILE}")
#	add_custom_command( OUTPUT ${OUTFILE}
#						COMMAND ${GLSLC} ${INFILE} "-o" ${OUTFILE}
#						VERBATIM
#						MAIN_DEPENDENCY "${INFILE}") 
#	add_dependencies(vkw_shaders ${OUTFILE})
#endforeach()