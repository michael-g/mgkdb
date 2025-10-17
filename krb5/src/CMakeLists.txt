
include_directories(${PROJECT_SOURCE_DIR}/include)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)

add_library(mgkrb5 SHARED mg_krb_auth.c)
target_link_libraries(mgkrb5 gssapi_krb5)
target_compile_options(mgkrb5 PRIVATE -Wall -Wextra -Wpedantic -Wno-unused-parameter -Wno-unused-variable -fPIC)

