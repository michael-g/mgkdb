function(add_core_test exec_name test_src)

    add_executable(${exec_name} ${test_src})

    target_link_libraries(${exec_name}
        PRIVATE
            GTest::gtest_main
            GTest::gmock
            ProjectOptions
            ${ARGN}
    )
    gtest_discover_tests(${exec_name})

endfunction()

#----------------------------------------------------------------------

add_core_test(MapTypeTest src/MapTypesTest.C MgMapTypes MgIoDefs MgIoMocks)

add_core_test(CircBufTest src/CircularBufferTest.C MgCircBuf MgIoDefs MgIoMocks)

add_core_test(CircBufITest src/CircularBufferIntegrationTest.C MgCircBuf MgIoDefs MgIoPosix)

add_core_test(FreeListTest src/FreeListTest.C MgCore MgFreeList)

add_core_test(CoreTest src/CoreTest.C MgIoMocks)

