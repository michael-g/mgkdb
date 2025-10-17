
include_directories(${PROJECT_SOURCE_DIR}/include)

add_library(mgio STATIC MgCircularBuffer.cpp)
target_sources(mgio PUBLIC
  FILE_SET public_headers
  TYPE HEADERS
  BASE_DIRS "${PROJECT_SOURCE_DIR}/include"
  FILES
    "../include/MgCircularBuffer.h"
    "../include/MgIoDefs.h"
 )

target_compile_options(mgio PRIVATE -Wall -Wextra -Wpedantic -Wno-unused-parameter -Wno-unused-variable -fPIC)

install(TARGETS mgio
  FILE_SET
    public_headers
  ARCHIVE
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
  INCLUDES
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
 )
