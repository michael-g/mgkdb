
include_directories(${PROJECT_SOURCE_DIR}/include)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Werror -fpie")

add_executable(coro tpmux.cpp)

