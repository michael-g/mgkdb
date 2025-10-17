cmake_minimum_required(VERSION 3.28)

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

# Controlled via environment variables:
# CMAKE_EXPORT_COMPILE_COMMANDS
# CMAKE_GNERATOR
# CMAKE_VERBOSE_MAKEFILE

set(CMAKE_CXX_STANDARD 23)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
# Project builds a static archive, but a little work would build a shared libary
#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)

# https://cmake.org/pipermail/cmake/2008-September/023808.html
IF(DEFINED CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
ELSE()
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
ENDIF()

if(CMAKE_BUILD_TYPE MATCHES Debug)
  message("   Using DEBUG build, setting -O0 -g")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g")
else()
  message("   Using RELEASE build, setting -O3")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -g")
endif()

add_subdirectory(src)
add_subdirectory(app)
add_subdirectory(examples)

enable_testing()
add_subdirectory(test)
