
add_library(MgKrb5 SHARED src/mg_krb5.c)

set(CMAKE_C_STANDARD 17)

target_link_libraries(MgKrb5
    PRIVATE
        ProjectOptions
        Kx
)

mg_cmake_install(LIB_NAME MgKrb5)

