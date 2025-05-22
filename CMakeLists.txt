cmake_minimum_required(VERSION 3.31 FATAL_ERROR)
project(metapool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Debug)
endif()


file(GLOB_RECURSE METAPOOL_SOURCES
	"${CMAKE_SOURCE_DIR}/src/*.cpp"
	"${CMAKE_SOURCE_DIR}/src/*.tpp"
	"${CMAKE_SOURCE_DIR}/main.cpp"
)


add_executable(metapool)

target_sources(metapool PRIVATE
	${METAPOOL_SOURCES}
)

target_include_directories(metapool PRIVATE
	"${CMAKE_SOURCE_DIR}/src"
	"${CMAKE_SOURCE_DIR}/benchmark"
	"${CMAKE_SOURCE_DIR}"
)

target_compile_features(metapool PRIVATE
	cxx_std_23
)

target_compile_options(metapool PRIVATE
	$<$<CONFIG:Debug>:-g>
	$<$<CONFIG:Debug>:-fdebug-prefix-map=${CMAKE_SOURCE_DIR}=.>
	$<$<CONFIG:Debug>:-Wall>
	$<$<CONFIG:Debug>:-Wextra>
	$<$<CONFIG:Debug>:-Wpedantic>
	$<$<CONFIG:Debug>:-Wshadow>
	$<$<CONFIG:Debug>:-Wconversion>
	$<$<CONFIG:Debug>:-Wsign-conversion>
	$<$<CONFIG:Release>:-O3>
	$<$<CONFIG:Release>:-DNDEBUG>
)

target_compile_definitions(metapool PRIVATE
	$<$<CONFIG:Debug>:ENABLE_ALLOC_LOGGING=1>
)


add_custom_target(run
	COMMAND $<TARGET_FILE:metapool> ${ARGS}
	DEPENDS metapool
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	VERBATIM
)
