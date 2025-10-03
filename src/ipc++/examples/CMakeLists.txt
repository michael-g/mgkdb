function(add_ipcpp_example exec_name test_src)

    add_executable(${exec_name} ${test_src} connect_to_kdb_lib.C)

    target_include_directories(${exec_name}
        PRIVATE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
    )

    target_link_libraries(${exec_name}
        PRIVATE
            ProjectOptions
            MgCore
            MgIoDefs
            MgIoPosix
            MgKdbIpcpp
            ${ARGN}
    )

endfunction()

#----------------------------------------------------------------------

# Ends up with an extraneous connect_to_kdb_lib.C compiled-in
add_ipcpp_example(journal_reader journal_reader.C)

add_ipcpp_example(connect_to_kdb connect_to_kdb_app.C MgCircBuf)

add_ipcpp_example(demo_circular_buffer demo_circular_buffer.C MgCircBuf)

add_ipcpp_example(subscribe_to_tp subscribe_to_tp.C MgCircBuf)

# Has a few too many libraries configured
add_ipcpp_example(variadic_printer variadic_printer.C)

