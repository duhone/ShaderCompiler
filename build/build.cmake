set(shader_compiler_root "${CMAKE_CURRENT_LIST_DIR}/..")

set(SHADER_COMPILER_SRCS
    ${shader_compiler_root}/src/main.cpp
)

set(SHADER_COMPILER_BUILD
    ${shader_compiler_root}/build/build.cmake
)

add_executable(shadercompiler  
	${SHADER_COMPILER_SRCS} 
	${SHADER_COMPILER_BUILD}
)
	
target_link_libraries(shadercompiler 
	fmt
	spdlog
	zstd
	core
	platform
	datacompression
)

source_group("Build" FILES ${SHADER_COMPILER_BUILD})
	
