
file(
  DOWNLOAD
  "https://github.com/KxSystems/kdb/raw/844514c58ba7c3e1d37995e48bf595d8da4cae5b/l64/c.o"
  "${CMAKE_BINARY_DIR}/c.o"
)

add_library(Kx_CdotO OBJECT IMPORTED GLOBAL)

set_property(TARGET Kx_CdotO
  PROPERTY IMPORTED_OBJECTS "${CMAKE_BINARY_DIR}/c.o"
)

# To reference c.o in a test, use something like this:
#add_executable(SomeKxLibTest
#  $<TARGET_OBJECTS:Kx_CdotO>
#  SomeKxLibTest.C ...
#)

add_subdirectory(core)
add_subdirectory(kx_systems)
add_subdirectory(ipc++)
add_subdirectory(krb5)
add_subdirectory(log_filter)
