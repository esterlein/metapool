cmake_minimum_required(VERSION 3.31 FATAL_ERROR)
project(mtp_benchmark LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_LINKER "lld")

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release CACHE STRING "build type" FORCE)
endif()

option(MTP_ENABLE_TRACE    "enable allocation trace" ON)
option(MTP_CONTAINERS_BOTH "compile mtp and std containers" ON)

file(GLOB MTP_SOURCES
	"${CMAKE_CURRENT_SOURCE_DIR}/benchmark_main.cpp"
)

add_executable(mtp_benchmark)

target_sources(mtp_benchmark PRIVATE ${MTP_SOURCES})

target_include_directories(mtp_benchmark PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}"
	"${CMAKE_CURRENT_SOURCE_DIR}/.."
	"${CMAKE_CURRENT_SOURCE_DIR}/../mtp"
	"${CMAKE_CURRENT_SOURCE_DIR}/../container"
)

target_compile_features(mtp_benchmark PRIVATE cxx_std_23)

target_compile_options(mtp_benchmark PRIVATE
	$<$<CONFIG:Debug>:-g>
	$<$<CONFIG:Debug>:-Wall>
	$<$<CONFIG:Debug>:-Wextra>
	$<$<CONFIG:Debug>:-Wpedantic>
	$<$<CONFIG:Debug>:-Wshadow>
	$<$<CONFIG:Debug>:-Wconversion>
	$<$<CONFIG:Debug>:-Wsign-conversion>

	$<$<CONFIG:Release>:-O3>
	$<$<CONFIG:Release>:-march=native>
	$<$<CONFIG:Release>:-fstrict-aliasing>
	$<$<CONFIG:Release>:-flto>
	$<$<CONFIG:Release>:-DNDEBUG>

	$<$<CONFIG:ReleaseWithDebugInfo>:-O3>
	$<$<CONFIG:ReleaseWithDebugInfo>:-march=native>
	$<$<CONFIG:ReleaseWithDebugInfo>:-fstrict-aliasing>
	$<$<CONFIG:ReleaseWithDebugInfo>:-flto>
	$<$<CONFIG:ReleaseWithDebugInfo>:-DNDEBUG>
	$<$<CONFIG:ReleaseWithDebugInfo>:-g>
)

if(MTP_ENABLE_TRACE)
	target_compile_definitions(mtp_benchmark PRIVATE MTP_ENABLE_TRACE=1)
endif()

if(MTP_CONTAINERS_BOTH)
	target_compile_definitions(mtp_benchmark PRIVATE MTP_CONTAINERS_BOTH=1)
endif()

add_custom_target(run
	COMMAND $<TARGET_FILE:mtp_benchmark> ${ARGS}
	DEPENDS mtp_benchmark
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	VERBATIM
)
