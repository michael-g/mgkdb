#----------------------------------------------------------------------

add_executable(CoroRingITest src/CoroRingITest.C)
target_compile_options(CoroRingITest PRIVATE -DMG_LOG_LVL=0)

find_package(PkgConfig REQUIRED)
pkg_check_modules(uring REQUIRED IMPORTED_TARGET liburing)

target_link_libraries(CoroRingITest
    PRIVATE
        ProjectOptions
        MgCore
        PkgConfig::uring
        GTest::gtest_main
        GTest::gmock
)

gtest_discover_tests(CoroRingITest)
