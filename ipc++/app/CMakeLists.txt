set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

include_directories(${PROJECT_SOURCE_DIR}/include)

add_executable(printer PrinterApp.cpp)
target_link_libraries(printer PRIVATE mgkdbipc)
target_compile_options(printer PRIVATE -Wall)

add_executable(writer WriterFun.cpp)
target_link_libraries(writer PRIVATE mgkdbipc)
target_compile_options(writer PRIVATE -Wall)
