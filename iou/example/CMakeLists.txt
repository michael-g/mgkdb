
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror")

add_library(mgio STATIC IMPORTED)
set_target_properties(mgio PROPERTIES
  IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/../io/lib/libmgio.a"
  INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/../io/include"
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(uring REQUIRED IMPORTED_TARGET liburing)

add_executable(iou_test
  mg_iou_test.cpp
)
target_link_libraries(iou_test PRIVATE PkgConfig::uring mgio)
