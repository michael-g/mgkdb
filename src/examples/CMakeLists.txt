function(add_example_app)
    cmake_parse_arguments(arg "" "APP_NAME" "SOURCES;DEPS" ${ARGN})

    add_executable(${arg_APP_NAME} ${arg_SOURCES})

    target_link_libraries(${arg_APP_NAME}
        PRIVATE
            ProjectOptions
            MgCore
            MgIoDefs
            MgIoPosix
            ${arg_DEPS}
    )

endfunction()

#----------------------------------------------------------------------

# Ends up with an extraneous connect_to_kdb_lib.C compiled-in
add_example_app(APP_NAME journal_reader
    SOURCES src/journal_reader.C src/connect_to_kdb_lib.C
    DEPS MgKdbIpcpp
)

add_example_app(APP_NAME connect_to_kdb
    SOURCES src/connect_to_kdb_app.C src/connect_to_kdb_lib.C
    DEPS MgCircBuf MgKdbIpcpp
)

add_example_app(APP_NAME demo_circular_buffer
    SOURCES src/demo_circular_buffer.C src/connect_to_kdb_lib.C
    DEPS MgCircBuf MgKdbIpcpp
)

add_example_app(APP_NAME subscribe_to_tp
    SOURCES src/subscribe_to_tp.C src/connect_to_kdb_lib.C
    DEPS MgCircBuf MgKdbIpcpp
)

# Has a few too many libraries configured
add_example_app(APP_NAME variadic_printer
    SOURCES src/variadic_printer.C
    DEPS MgKdbIpcpp
)

add_example_app(APP_NAME io_uring_circ_buf_test
    SOURCES src/mg_iou_test.C src/connect_to_kdb_lib.C
    DEPS MgCircBuf MgKdbIpcpp
)
find_package(PkgConfig REQUIRED)
pkg_check_modules(uring REQUIRED IMPORTED_TARGET liburing)
target_link_libraries(io_uring_circ_buf_test
    PRIVATE PkgConfig::uring
)
