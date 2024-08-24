
include_directories(${PROJECT_SOURCE_DIR}/include)

# To build both a static and dynamic library you can uncomment the following lines.
# You will need to comment-out the add_library instruction that creates the static
# (and non-PIC) library or you'll get a naming collision.
#
# C.f. https://cmake.org/cmake/help/latest/command/add_library.html#object-libraries
# and https://stackoverflow.com/a/29824424/322304
#
# add_library(objlib OBJECT KdbType.cpp)
# set_property(TARGET objlib PROPERTY POSITION_INDEPENDENT_CODE 1)
# target_compile_options(objlib PRIVATE -Wall -Wextra -Wpedantic -O2 -Wno-unused-parameter -Wno-unused-variable)
# add_library(mgkdbipc        SHARED $<TARGET_OBJECTS:objlib>)
# add_library(mgkdbipc_static STATIC $<TARGET_OBJECTS:objlib>)

add_library(mgkdbipc STATIC KdbType.cpp)
target_compile_options(mgkdbipc PRIVATE -Wall -Wextra -Wpedantic -O2 -Wno-unused-parameter -Wno-unused-variable)
