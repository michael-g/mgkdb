
# To download, build and install cppcoro:
# git clone --recursive git@github.com:andreasbuhr/cppcoro.git cppcoro
# cd cppcoro
# CMAKE_INSTALL_PREFIX=~/alt cmake -S . -B build
# cmake --install build
find_package(cppcoro REQUIRED)

function(add_tpmux_props lib_name)
    set_target_properties(${lib_name}
        PROPERTIES
            CMAKE_CXX_COMPILER "/usr/lib64/ccache/clang++"
            CMAKE_C_COMPILER "/usr/lib64/ccache/clang"
            CMAKE_INSTALL_PREFIX "$HOME/alt"
    )
endfunction()

#----------------------------------------------------------------------

add_library(MgTpmuxLib STATIC
  src/mg_coro_tcp_connect.cpp
  src/mg_coro_kdb_connect.cpp
  src/mg_coro_kdb_read_msg.cpp
  src/mg_coro_epoll.cpp
  src/mg_coro_kdb_subscribe_replay.cpp
  src/mg_coro_kdb_recv_tcp_msgs.cpp
)

add_tpmux_props(MgTpmuxLib)

target_include_directories(MgTpmuxLib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(MgTpmuxLib MgKdbIpcpp MgCore MgIoDefs MgIoPosix cppcoro::cppcoro)

mg_cmake_install(LIB_NAME MgTpmuxLib)

#----------------------------------------------------------------------

add_executable(MgTpmux
  src/mg_coro_app.cpp
)
add_tpmux_props(MgTpmux)

target_link_libraries(MgTpmux MgTpmuxLib)

mg_cmake_install(LIB_NAME MgTpmux)

#add_subdirectory(test)
