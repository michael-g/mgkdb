cmake_minimum_required(VERSION 3.28)

# prohibit in-source-builds
if (${CMAKE_BINARY_DIR} STREQUAL ${CMAKE_SOURCE_DIR})
  message(STATUS "In-source-builds are not allowed")
  message(STATUS "Don't forget to clean up the mess created, e.g. there's an errant CmakeCache.txt file")
  message(STATUS "knocking around.")
  message(FATAL_ERROR "Please create a separate build directory and call CMake again")
endif()

set(TARGET mgreplay)

project(${TARGET} LANGUAGES C)

set(CMAKE_C_STANDARD     11)
set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR})

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)

# https://cmake.org/pipermail/cmake/2008-September/023808.html
IF(DEFINED CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE  ${CMAKE_BUILD_TYPE} CACHE STRING "Choose the type of build, options are: None(CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
ELSE()
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build, options are: None(CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
ENDIF()


if(CMAKE_BUILD_TYPE MATCHES Debug)
  message("   Using DEBUG build, setting -O0 -g")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g")
else()
  message("   Using RELEASE build, setting -O3")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")  # -flto")
endif()

add_compile_definitions(_GNU_SOURCE)
add_compile_definitions(_FILE_OFFSET_BITS=64)
add_compile_definitions(_LARGEFILE64_SOURCE)

add_subdirectory(src)

