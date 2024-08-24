
include_directories(${PROJECT_SOURCE_DIR}/include)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall -Werror -fPIC")
add_library(mglogfilter SHARED log_filter.c)

