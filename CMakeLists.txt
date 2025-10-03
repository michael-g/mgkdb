cmake_minimum_required(VERSION 3.28)

# prohibit in-source-builds
if(${CMAKE_BINARY_DIR} STREQUAL ${CMAKE_SOURCE_DIR})
  message(STATUS "In-source-builds are not allowed.")
  message(STATUS "Don't forget to clean up the mess created...")
  message(FATAL_ERROR "Please create a separate build directory and call CMake again")
endif()

# speed up recompiling on unix with ccache
set(TARGET MgKdb)

project(${TARGET}
  VERSION 1.0
  LANGUAGES CXX C
  HOMEPAGE_URL "%%github.com/michaelg-7x/TODO"
)

# Controlled via environment variables:
# CMAKE_EXPORT_COMPILE_COMMANDS
# CMAKE_GENERATOR
# CMAKE_VERBOSE_MAKEFILE

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include module for GNU standard installation directories
include(GNUInstallDirs)

include(cmake/StandardProjectSettings.cmake)
include(cmake/CustomFunctions.cmake)
include(cmake/ProjectOptions.cmake)

find_package(GTest REQUIRED)
include(GoogleTest)

enable_testing()

add_subdirectory(src)
