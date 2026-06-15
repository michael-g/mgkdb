
add_executable(MgTpmuxTest src/test_mg_coro_tcp_connect.cpp)

target_link_libraries(MgTpmuxTest
    PRIVATE
        GTest::gtest_main
        GTest::gmock
        ProjectOptions
        MgTpmuxLib
)

gtest_discover_tests(MgTpmuxTest)

