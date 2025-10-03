function(add_ipcpp_test exec_name test_src)

    add_executable(${exec_name} ${test_src})

    target_link_libraries(${exec_name}
        PRIVATE
            GTest::gtest_main
            GTest::gmock
            ProjectOptions
            MgIoMocks
            MgKdbIpcpp
            ${ARGN} # not currently used, but future-proofing etc
    )
    gtest_discover_tests(${exec_name})

endfunction()

#----------------------------------------------------------------------
add_ipcpp_test(KdbTypeTest src/KdbTypeTest.C)

add_ipcpp_test(KdbIpcMessageReaderTest src/KdbIpcMessageReaderTest.C)

add_ipcpp_test(ConnectToKdbITest src/ConnectToKdbTest.C)
