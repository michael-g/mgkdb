set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

include_directories(${PROJECT_SOURCE_DIR}/include)

add_executable(journal_reader journal_reader.cpp)
target_link_libraries(journal_reader PRIVATE mgkdbipc)
target_compile_options(journal_reader PRIVATE -Wall)

add_executable(connect_to_kdb connect_to_kdb_lib.cpp connect_to_kdb_app.cpp)
target_link_libraries(connect_to_kdb PRIVATE mgkdbipc)
target_compile_options(connect_to_kdb PRIVATE -Wall)

add_executable(demo_circular_buffer connect_to_kdb_lib.cpp demo_circular_buffer.cpp)
target_link_libraries(demo_circular_buffer PRIVATE mgkdbipc)
target_compile_options(demo_circular_buffer PRIVATE -Wall)

add_executable(subscribe_to_tp connect_to_kdb_lib.cpp subscribe_to_tp.cpp)
target_link_libraries(subscribe_to_tp PRIVATE mgkdbipc)
target_compile_options(subscribe_to_tp PRIVATE -Wall)

add_executable(variadic_printer connect_to_kdb_lib.cpp variadic_printer.cpp)
target_link_libraries(variadic_printer PRIVATE mgkdbipc)
target_compile_options(variadic_printer PRIVATE -Wall)
