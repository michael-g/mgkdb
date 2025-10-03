add_library(Kx INTERFACE)

target_include_directories(Kx INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(Kx INTERFACE ProjectOptions)

mg_cmake_install(LIB_NAME Kx)

