set(root "${CMAKE_CURRENT_LIST_DIR}/..")

set(SRCS
    ${root}/src/main.cpp
)

set(BUILD
    ${root}/build/build.cmake
)

set(TEST_FILES
    ${root}/assets/shader_modules/shader.vert
    ${root}/assets/shader_modules/shader.frag
)

add_executable(shadercompiler  
	${SRCS} 
	${BUILD}
	${TEST_FILES}
)
	
settingsCR(shadercompiler)	
usePCH(shadercompiler core)
		
target_link_libraries(shadercompiler 
	cli11
	doctest
	fmt
	function2
	spdlog
	zstd
	core
	platform
	datacompression
)

source_group("Test Shaders" FILES ${TEST_FILES})
	
add_custom_command(TARGET shadercompiler POST_BUILD        
  COMMAND ${CMAKE_COMMAND} -E copy_if_different  
      ${TEST_FILES}
      $<TARGET_FILE_DIR:shadercompiler>)
			
