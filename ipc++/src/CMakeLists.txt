
include_directories(${PROJECT_SOURCE_DIR}/include)

# To build both a static and dynamic library you can uncomment the following lines.
# You will need to comment-out the add_library instruction that creates the static
# (and non-PIC) library or you'll get a naming collision.
#
# C.f. https://cmake.org/cmake/help/latest/command/add_library.html#object-libraries
# and https://stackoverflow.com/a/29824424/322304
#
# add_library(objlib OBJECT KdbType.cpp)
# set_property(TARGET objlib PROPERTY POSITION_INDEPENDENT_CODE 1)
# target_compile_options(objlib PRIVATE -Wall -Wextra -Wpedantic -Wno-unused-parameter -Wno-unused-variable)
# add_library(mgkdbipc        SHARED $<TARGET_OBJECTS:objlib>)
# add_library(mgkdbipc_static STATIC $<TARGET_OBJECTS:objlib>)

add_library(mgio STATIC IMPORTED)
set_target_properties(mgio PROPERTIES
  IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/../io/lib/libmgio.a"
  INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/../io/include"
)

add_library(mgkdbipc STATIC KdbType.cpp)
target_link_libraries(mgkdbipc PUBLIC mgio)
target_compile_options(mgkdbipc PRIVATE -Wall -Wextra -Wpedantic -Wno-unused-parameter -Wno-unused-variable -fPIC)
target_sources(mgkdbipc PUBLIC
  FILE_SET public_headers
  TYPE HEADERS
  BASE_DIRS
    "${PROJECT_SOURCE_DIR}/include"
  FILES
    "../include/KdbType.h"
 )

install(TARGETS mgkdbipc
  FILE_SET
    public_headers
  ARCHIVE
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
  INCLUDES
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
 )
