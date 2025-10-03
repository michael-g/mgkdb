add_library(MgLogFilter SHARED src/log_filter.c)

set(CMAKE_C_STANDARD 17)

target_link_libraries(MgLogFilter
    PRIVATE
        ProjectOptions
    PUBLIC
        MgCore
        Kx
)
# Need to remove compiler option '-Woverloaded-virtual=2'

mg_cmake_install(LIB_NAME MgLogFilter)


