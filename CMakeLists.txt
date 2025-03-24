cmake_minimum_required(VERSION 3.31 FATAL_ERROR)

project(metapool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Debug)
endif()

add_compile_options(
	-Wall
	-Wextra
	-Wpedantic
	-Wshadow
	-Wconversion
	-Wsign-conversion
#	-Werror
)

set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")
file(GLOB_RECURSE SOURCES ${SRC_DIR}/*.cpp ${SRC_DIR}/*.tpp)
add_executable(metapool ${SOURCES})
target_include_directories(metapool PRIVATE ${SRC_DIR})

add_custom_target(run
	COMMAND $<TARGET_FILE:metapool> ${ARGS}
	DEPENDS metapool
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	COMMENT "running metapool..."
	VERBATIM
)
