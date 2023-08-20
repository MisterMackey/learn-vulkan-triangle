cmake_minimum_required(VERSION 3.2.0)
project(Triangle)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_link_options(-lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi)

file(GLOB_RECURSE GLSL_SOURCE_FILES
	"shaders/*.frag"
	"shaders/*.vert"
)

foreach(GLSL ${GLSL_SOURCE_FILES})
	get_filename_component(FILE_NAME ${GLSL} NAME)
	set(SPIRV "${PROJECT_BINARY_DIR}/shaders/${FILE_NAME}.spv")
	add_custom_command(
		OUTPUT ${SPIRV}
		COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/shaders/"
		COMMAND glslangValidator -V ${GLSL} -o ${SPIRV}
		DEPENDS ${GLSL}
	)
	list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(Shaders DEPENDS ${SPIRV_BINARY_FILES})

add_executable(Triangle main.cpp debugshit.cpp p_device.cpp requirement.cpp presentation.cpp)

add_dependencies(Triangle Shaders)

add_test(NAME Triangle COMMAND ./Triangle)