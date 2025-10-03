add_library(ProjectOptions INTERFACE)

include(${PROJECT_SOURCE_DIR}/cmake/CompilerWarnings.cmake)

set_project_warnings(ProjectOptions)

include(GNUInstallDirs)

if(CMAKE_BUILD_TYPE MATCHES Debug)
  message("   Using DEBUG build, setting -O0 -g")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Og -g")
else()
  message("   Using RELEASE build, setting -O3")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
endif()

# Install project options library and export as set
install(TARGETS
  ProjectOptions
  EXPORT ProjectOptionsExportSet
)

# Install the project options export set
install(EXPORT ProjectOptionsExportSet
  FILE ProjectOptionsTargets.cmake
  NAMESPACE ProjectOptions::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ProjectOptions
)
