find_package(GTest CONFIG REQUIRED)

include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/src)

add_library(mgio STATIC IMPORTED)
set_target_properties(mgio PROPERTIES
  IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/../io/lib/libmgio.a"
  INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/../io/include"
)

function(add_test_file test_src)
	get_filename_component(src_base ${test_src} NAME_WE)
	set(test_libs GTest::gtest GTest::gmock mgio)
	message(" * Adding test source ${test_src} with libs ${test_libs}")
	add_executable(${src_base} ${test_src})
	add_compile_options(-O0 -g)
	target_link_libraries(${src_base} ${test_libs})
	add_test(NAME ${src_base} COMMAND ${CMAKE_BINARY_DIR}/test/${src_base})
endfunction()

# How to set RPATH in a library
# https://stackoverflow.com/a/37684619/322304
# How to use LD_PRELOAD with CMake
# https://stackoverflow.com/a/48203323/322304
# https://stackoverflow.com/a/3411412/322304
# https://www.quora.com/How-can-I-mock-system-call-error-condition-in-a-C++-unit-test
add_test_file(KdbTypeTest.cpp)
add_test_file(ConnectToKdbTest.cpp)
add_test_file(KdbIpcMessageReaderTest.cpp)
