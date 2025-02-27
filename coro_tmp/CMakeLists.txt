cmake_minimum_required(VERSION 3.28)

# prohibit in-source-builds
if (${CMAKE_BINARY_DIR} STREQUAL ${CMAKE_SOURCE_DIR})
	message(STATUS "In-source-builds are not allowed")
	message(STATUS "Don't forget to clean up the mess created, e.g. there's an errant CmakeCache.txt file")
	message(STATUS "knocking around.")
	message(FATAL_ERROR "Please create a separate build directory and call CMake again")
endif()

set(TARGET tpmux)

project(${TARGET} LANGUAGES C CXX)

set(CMAKE_CXX_COMPILER "/home/michaelg/opt/bin/clang++")
set(CMAKE_C_COMPILER "/home/michaelg/opt/bin/clang")
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g -Xclang -disable-O0-optnone -emit-llvm -S")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g -Xclang -disable-O0-optnone -emit-llvm -S")
set(CMAKE_CXX_STANDARD     23)
set(CMAKE_C_STANDARD       17)
set(CMAKE_INSTALL_PREFIX   ${PROJECT_SOURCE_DIR})

set(CMAKE_VERBOSE_MAKEFILE OFF)
option(SAVE_TEMPS "Save temporary files" ON)
#set(SAVE_TEMPS true CACHE BOOL "Save temporary files")

# this from https://github.com/ycm-core/YouCompleteMe#user-guide
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)

# https://cmake.org/pipermail/cmake/2008-September/023808.html
IF(DEFINED CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE  ${CMAKE_BUILD_TYPE} CACHE STRING "Choose the type of build, options are: None(CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
ELSE()
	set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build, options are: None(CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
ENDIF()

# TODO this doesn't work as I'd expect, it's always buliding the RELEASE version
if(CMAKE_BUILD_TYPE MATCHES Debug)
	message(" * Using DEBUG build, setting -O0 -g")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g")
else()
	message(" * Using RELEASE build, setting -O3")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")  # -flto")
endif()

# add_compile_definitions(_GNU_SOURCE)
# add_compile_definitions(_FILE_OFFSET_BITS=64)
# add_compile_definitions(_LARGEFILE64_SOURCE)

# TODO this doesn't work
if(${SAVE_TEMPS})
	message(" * Saving temp-files")
	add_compile_options(-save-temps)
endif(${SAVE_TEMPS})

# add 'test' target to Makefiles; this enables testing for this directory and below
# https://cmake.org/cmake/help/latest/command/enable_testing.html
#enable_testing()

add_subdirectory(src)
#add_subdirectory(test)

