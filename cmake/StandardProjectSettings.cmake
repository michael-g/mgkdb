
# https://cmake.org/pipermail/cmake/2008-September/023808.html
if(DEFINED CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
else()
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
endif()

# Specify the output directory for executables
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Specify the output directory for libraries
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
  message(STATUS "ccache found")
  # Support Unix Makefiles and Ninja
  #set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
  set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
else()
  message(STATUS "ccache not found")
endif()

# Include Interprocedural Optimisation
include(CheckIPOSupported)

check_ipo_supported(RESULT result OUTPUT output)
if(result)
  message(STATUS "Using ITO")
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
  #set_property(TARGET ${TARGET} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
  message(WARNING "IPO is not supported: ${output}")
endif()

# Generate and install project config
include(${PROJECT_SOURCE_DIR}/cmake/GenerateAndInstallConfig.cmake)
