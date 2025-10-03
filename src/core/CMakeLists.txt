# C.f. https://github.com/gregorykelleher/InterviewPracticeQuestions/blob/fb7680227ae5f0f1bc825c92b20e44e5957f9cae/src/anagram/CMakeLists.txt

#----------------------------------------------------------------- MgCore
# Library MgCore exports headers:
# . MgCore.H
# . MgDebug.H
add_library(MgCore INTERFACE)

target_sources(MgCore
    INTERFACE
    FILE_SET mg_core_fs
    TYPE HEADERS
    BASE_DIRS
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    FILES
        include/MgCore.H
        include/MgDebug.H
)

target_link_libraries(MgCore INTERFACE ProjectOptions)

mg_cmake_install(LIB_NAME MgCore FS_NAME mg_core_fs)
#----------------------------------------------------------------- MgIoDefs
# Library MgIoDefs exports the header
# . MgIoDefs.H
# Use this in conjunction with _either_ MgIoPosix or MgIoMocks in the
# eventual binary/shared library
add_library(MgIoDefs INTERFACE)
target_sources(MgIoDefs
    INTERFACE
        FILE_SET mg_io_defs_fs
        TYPE HEADERS
        BASE_DIRS
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
        FILES
            include/MgIoDefs.H
)
target_link_libraries(MgIoDefs INTERFACE ProjectOptions)

mg_cmake_install(LIB_NAME MgIoDefs FS_NAME mg_io_defs_fs)

#---------------------------------------------------------------- MgIoPosix
# Provides a real implementation for the MgIoDefs header
# C.f. MgIoMocks for the fake implemenetation
add_library(MgIoPosix STATIC)

target_sources(MgIoPosix
    PRIVATE
        src/PosixWrappers.C
        src/ReadFullyImpl.C
    PUBLIC
        FILE_SET mg_io_posix_fs
        TYPE HEADERS
        BASE_DIRS
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
        FILES
            include/MgIoDefs.H
)

target_link_libraries(MgIoPosix
    PRIVATE ProjectOptions
    PUBLIC MgCore
)

mg_cmake_install(LIB_NAME MgIoPosix FS_NAME mg_io_posix_fs)
#------------------------------------------------------------- Io Test Defs
# Provides GMock mocks for the MgIoDefs header
# C.f. MgIoPosix for the real implemenetation
add_library(MgIoMocks STATIC)

target_sources(MgIoMocks
    PRIVATE
        src/PosixWrapperMocks.C
        src/ReadFullyImpl.C
    PUBLIC
        FILE_SET mg_io_mocks_fs_defs
        TYPE HEADERS
        BASE_DIRS
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include-mocks>
            $<INSTALL_INTERFACE:include>
            $<INSTALL_INTERFACE:include-mocks>
        FILES
            include/MgIoDefs.H
            include-mocks/MockMgIoDefs.H
)

target_link_libraries(MgIoMocks
  PRIVATE
    ProjectOptions
    MgCore
    GTest::gmock
)

mg_cmake_install(LIB_NAME MgIoMocks FS_NAME mg_io_mocks_fs_defs)

#--------------------------------------------------------------- MgMapTypes
# Exports a library of types which wrap various mmap operations. It depends
# on the MgIoDefs library, so linking against this requires you to link
# against either MgIoPosix or MgIoMocks to get an implementation for the
# IO functions.
add_library(MgMapTypes STATIC)

target_sources(MgMapTypes
    PRIVATE
        src/MapTypes.C
    PUBLIC
        FILE_SET mg_map_types_fs_defs
        TYPE HEADERS
        BASE_DIRS
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
        FILES
            include/MgMapTypes.H
)

target_link_libraries(MgMapTypes
    PRIVATE
        ProjectOptions
    PUBLIC
        MgIoDefs
        MgCore
)

target_link_libraries(MgMapTypes PUBLIC MgCore)

mg_cmake_install(LIB_NAME MgMapTypes FS_NAME mg_map_types_fs_defs)

#----------------------------------------------------------- CircularBuffer
add_library(MgCircBuf STATIC)

target_sources(MgCircBuf
    PRIVATE
        src/CircularBuffer.C
    PUBLIC
        FILE_SET mg_circ_buf_fs
        TYPE HEADERS
        BASE_DIRS
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
        FILES
            include/MgCircularBuffer.H
)

target_link_libraries(MgCircBuf
    PRIVATE
        ProjectOptions
    PUBLIC
        MgCore
        MgIoDefs
)

mg_cmake_install(LIB_NAME MgCircBuf FS_NAME mg_circ_buf_fs)
#----------------------------------------------------------------- FreeList
add_library(MgFreeList STATIC)

target_sources(MgFreeList
    PRIVATE
        src/FreeList.C
    PUBLIC
        FILE_SET mg_free_list_fs
        TYPE HEADERS
        BASE_DIRS
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
        FILES
            include/MgFreeList.H
)

target_link_libraries(MgFreeList
    PRIVATE
        ProjectOptions
    PUBLIC
        MgCore
)

mg_cmake_install(LIB_NAME MgFreeList FS_NAME mg_free_list_fs)

#----------------------------------------------------------------- tests
add_subdirectory(test)
