cmake_minimum_required(VERSION 3.31 FATAL_ERROR)

project(metapool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -fdebug-prefix-map=${CMAKE_SOURCE_DIR}=.")

add_compile_options(
	-Wall
	-Wextra
	-Wpedantic
	-Wshadow
	-Wconversion
	-Wsign-conversion
	-g
#	-Werror
)

find_library(PROFILER_LIBRARY profiler PATHS /usr/local/lib /usr/local/opt/gperftools/lib)
if(NOT PROFILER_LIBRARY)
	message(FATAL_ERROR "could not find libprofiler from gperftools")
endif()
message(STATUS "found profiler library: ${PROFILER_LIBRARY}")

set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")
set(BENCHMARK_DIR "${CMAKE_SOURCE_DIR}/benchmark")
file(GLOB_RECURSE SOURCES ${SRC_DIR}/*.cpp ${SRC_DIR}/*.tpp)

list(APPEND SOURCES "${CMAKE_SOURCE_DIR}/main.cpp")

add_executable(metapool ${SOURCES})
target_include_directories(metapool PRIVATE
	${SRC_DIR}
	${BENCHMARK_DIR}
	${CMAKE_SOURCE_DIR}
)

target_link_libraries(metapool ${PROFILER_LIBRARY})

add_custom_target(run
	COMMAND $<TARGET_FILE:metapool> ${ARGS}
	DEPENDS metapool
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	COMMENT "running metapool..."
	VERBATIM
)

add_custom_target(profile
	COMMAND ${CMAKE_COMMAND} -E env CPUPROFILE=metapool.prof $<TARGET_FILE:metapool> ${ARGS}
	DEPENDS metapool
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	COMMENT "profiling metapool..."
	VERBATIM
)

add_custom_target(analyze
	COMMAND pprof --text $<TARGET_FILE:metapool> ${CMAKE_CURRENT_BINARY_DIR}/metapool.prof
	DEPENDS profile
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	COMMENT "analyzing metapool profile..."
	VERBATIM
)

add_custom_target(analyze-web
	COMMAND pprof --web $<TARGET_FILE:metapool> ${CMAKE_CURRENT_BINARY_DIR}/metapool.prof
	DEPENDS profile
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	COMMENT "web visualization of metapool profile..."
	VERBATIM
)
