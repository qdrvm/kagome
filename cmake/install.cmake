### kagome_install should be called right after add_library(target)
function(kagome_install target)
    install(TARGETS ${target} EXPORT kagomeTargets
        LIBRARY       DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE       DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME       DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FRAMEWORK     DESTINATION ${CMAKE_INSTALL_PREFIX}
        )
endfunction()

### workaround for imported libraries
function(kagome_install_mini target)
    install(TARGETS ${target} EXPORT kagomeTargets
        LIBRARY       DESTINATION ${CMAKE_INSTALL_LIBDIR}
        )
endfunction()

function(kagome_install_setup)
    set(options)
    set(oneValueArgs)
    set(multiValueArgs HEADER_DIRS)
    cmake_parse_arguments(arg "${options}" "${oneValueArgs}"
        "${multiValueArgs}" ${ARGN})

    foreach (DIR IN ITEMS ${arg_HEADER_DIRS})
        get_filename_component(FULL_PATH ${DIR} ABSOLUTE)
        string(REPLACE ${CMAKE_CURRENT_SOURCE_DIR}/core "." RELATIVE_PATH ${FULL_PATH})
        get_filename_component(INSTALL_PREFIX ${RELATIVE_PATH} DIRECTORY)
        install(DIRECTORY ${DIR}
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${INSTALL_PREFIX}
            FILES_MATCHING PATTERN "*.hpp")
    endforeach ()

    install(
        EXPORT kagomeTargets
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/kagome
        NAMESPACE kagome::
    )
    export(
        EXPORT kagomeTargets
        FILE ${CMAKE_CURRENT_BINARY_DIR}/kagomeTargets.cmake
        NAMESPACE kagome::
    )

endfunction()
