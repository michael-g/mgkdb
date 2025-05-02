
include_directories(${PROJECT_SOURCE_DIR}/include)

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror -fPIC")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror -fPIE")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -fPIE")

add_library(kdbipc STATIC IMPORTED)

set_target_properties(kdbipc PROPERTIES
  IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/../ipc++/lib/libmgkdbipc.a"
  INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/../ipc++/include"
)

add_library(cppcoro STATIC IMPORTED)
set_target_properties(cppcoro PROPERTIES
  IMPORTED_LOCATION "/home/michaelg/alt/lib/libcppcoro.a"
  INTERFACE_INCLUDE_DIRECTORIES "/home/michaelg/alt/include"
)

add_executable(tpmux mg_coro_app.cpp mg_coro_kdb_connect.cpp mg_coro_tcp_connect.cpp mg_coro_kdb_read_msg.cpp mg_coro_epoll.cpp mg_coro_kdb_subscribe.cpp)
target_link_libraries(tpmux kdbipc cppcoro)
