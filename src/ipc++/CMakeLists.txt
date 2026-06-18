
add_library(MgKdbIpcpp STATIC src/KdbType.C)

target_include_directories(MgKdbIpcpp
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(MgKdbIpcpp
    PRIVATE
        ProjectOptions
        MgIoDefs
)

mg_cmake_install(LIB_NAME MgKdbIpcpp)

#------------------------------------------------------------------------- Tests
add_subdirectory(test)
