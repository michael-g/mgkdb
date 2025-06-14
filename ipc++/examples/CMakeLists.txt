set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

include_directories(${PROJECT_SOURCE_DIR}/include)

add_executable(journal_reader journal_reader.cpp)
target_link_libraries(journal_reader PRIVATE mgkdbipc)
target_compile_options(journal_reader PRIVATE -Wall)

