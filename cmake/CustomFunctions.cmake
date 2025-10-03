include(CMakeParseArguments)

function(mg_cmake_install)
    cmake_parse_arguments(arg "" "LIB_NAME;FS_NAME" "" ${ARGN})
    if("${arg_FS_NAME}" STREQUAL "")
        install(TARGETS ${arg_LIB_NAME}
            EXPORT ${arg_LIB_NAME}ExportSet
        )
    else()
        install(TARGETS ${arg_LIB_NAME}
            EXPORT ${arg_LIB_NAME}ExportSet
            FILE_SET ${arg_FS_NAME}
        )
    endif()

    install(EXPORT ${arg_LIB_NAME}ExportSet
        FILE ${arg_LIB_NAME}OpsTargets.cmake
        NAMESPACE ${arg_LIB_NAME}::
        DESTINATION ${CMAKE_INSTALL_DIR}/cmake/${arg_LIB_NAME}
    )
endfunction()


