cmake_minimum_required(VERSION 3.10)

# prohibit in-source-builds
if (${CMAKE_BINARY_DIR} STREQUAL ${CMAKE_SOURCE_DIR})
	message(STATUS "In-source-builds are not allowed")
	message(STATUS "Don't forget to clean up the mess created, e.g. there's an errant CmakeCache.txt file knocking around.")
	message(FATAL_ERROR "Please create a separate build directory and call CMake again")
endif()

# speed up recompiling on unix with ccache
find_program(CCACHE_PROGRAM ccache)
if (CCACHE_PROGRAM)
	message(STATUS "ccache found")
	# Support Unix Makefiles and Ninja
	set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()

set(TARGET MgKdbIpc)

project(${TARGET} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_VERBOSE_MAKEFILE OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)

option(SAVE_TEMPS "Save temporary files" ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)  # this from https://github.com/ycm-core/YouCompleteMe#user-guide

# https://cmake.org/pipermail/cmake/2008-September/023808.html
IF(DEFINED CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
ELSE()
	set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
ENDIF()

if(CMAKE_BUILD_TYPE MATCHES Debug)
	message(" * Using DEBUG build, setting -O0 -g")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g")
else()
	message(" * Using RELEASE build, setting -O3")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g")
endif()

if(${SAVE_TEMPS})
	add_compile_options(-save-temps)
endif(${SAVE_TEMPS})

add_subdirectory(src)
add_subdirectory(app)

enable_testing()
add_subdirectory(test)
