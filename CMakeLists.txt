cmake_minimum_required(VERSION 3.31 FATAL_ERROR)
project(metapool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

find_library(PROFILER_LIBRARY profiler PATHS
	/usr/local/lib
	/usr/local/opt/gperftools/lib
	REQUIRED
)
message(STATUS "found profiler library: ${PROFILER_LIBRARY}")

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

target_link_libraries(metapool PRIVATE
	${PROFILER_LIBRARY}
)

add_custom_target(run
	COMMAND $<TARGET_FILE:metapool> ${ARGS}
	DEPENDS metapool
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	VERBATIM
)

add_custom_target(profile
	COMMAND ${CMAKE_COMMAND} -E env CPUPROFILE=metapool.prof $<TARGET_FILE:metapool> ${ARGS}
	DEPENDS metapool
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	VERBATIM
)

add_custom_target(analyze
	COMMAND ${CMAKE_SOURCE_DIR}/perft_setup.zsh $<TARGET_FILE:metapool> ${CMAKE_CURRENT_BINARY_DIR}/metapool.prof
	DEPENDS profile
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	VERBATIM
)

add_custom_target(analyze-web
	COMMAND ${CMAKE_COMMAND} -E env
		DYLD_LIBRARY_PATH=/usr/local/opt/llvm/lib:/usr/local/opt/llvm/lib/c++
		pprof
		--web
		$<TARGET_FILE:metapool>
		${CMAKE_CURRENT_BINARY_DIR}/metapool.prof
	DEPENDS profile
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	VERBATIM
)
