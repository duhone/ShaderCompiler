set(shader_compiler_root "${CMAKE_CURRENT_LIST_DIR}/..")

set(SHADER_COMPILER_SRCS
    ${shader_compiler_root}/src/main.cpp
)

set(SHADER_COMPILER_BUILD
    ${shader_compiler_root}/build/build.cmake
)

set(SHADER_COMPILER_TEST_FILES
    ${shader_compiler_root}/assets/shader_modules/shader.vert
    ${shader_compiler_root}/assets/shader_modules/shader.frag
)

add_executable(shadercompiler  
	${SHADER_COMPILER_SRCS} 
	${SHADER_COMPILER_BUILD}
	${SHADER_COMPILER_TEST_FILES}
)
	
target_link_libraries(shadercompiler 
	cli11
	fmt
	spdlog
	zstd
	core
	platform
	datacompression
)

source_group("Build" FILES ${SHADER_COMPILER_BUILD})
source_group("Test Shaders" FILES ${SHADER_COMPILER_TEST_FILES})
	
add_custom_command(TARGET shadercompiler POST_BUILD        
  COMMAND ${CMAKE_COMMAND} -E copy_if_different  
      ${shader_compiler_root}/assets/shader_modules/shader.vert
      $<TARGET_FILE_DIR:shadercompiler>)
			
add_custom_command(TARGET shadercompiler POST_BUILD        
  COMMAND ${CMAKE_COMMAND} -E copy_if_different  
      ${shader_compiler_root}/assets/shader_modules/shader.frag
      $<TARGET_FILE_DIR:shadercompiler>)
