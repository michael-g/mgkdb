
include_directories(${PROJECT_SOURCE_DIR}/include)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror")

add_library(mgio STATIC IMPORTED)
set_target_properties(mgio PROPERTIES
  IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/../io/lib/libmgio.a"
  INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/../io/include"
)

#add_library(io_uring STATIC IMPORTED)
#set_target_properties(io_uring PROPERTIES
#  IMPORTED_LOCATION "/some/path/to/liburing/src/liburing.a"
#  INTERFACE_INCLUDE_DIRECTORIES "/some/path/to/liburing/include/liburing.h"
#)

find_package(PkgConfig REQUIRED)
pkg_check_modules(uring REQUIRED IMPORTED_TARGET liburing)

add_executable(iou_test
  mg_iou_test.cpp
)
target_link_libraries(iou_test PRIVATE PkgConfig::uring mgio)
