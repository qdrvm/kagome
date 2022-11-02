### kagome_install should be called right after add_library(target)
function(kagome_install target)
    install(TARGETS ${target} EXPORT kagomeTargets
        LIBRARY       DESTINATION ${CMAKE_INSTALL_LIBDIR}/kagome
        ARCHIVE       DESTINATION ${CMAKE_INSTALL_LIBDIR}/kagome
        RUNTIME       DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/kagome
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/kagome
        FRAMEWORK     DESTINATION ${CMAKE_INSTALL_PREFIX}/kagome
        )
endfunction()

### workaround for imported libraries
function(kagome_install_mini target)
    install(TARGETS ${target} EXPORT kagomeTargets
        LIBRARY       DESTINATION ${CMAKE_INSTALL_LIBDIR}/kagome
        )
endfunction()

function(kagome_install_setup)
    set(options)
    set(oneValueArgs)
    set(multiValueArgs HEADER_DIRS)
    cmake_parse_arguments(arg "${options}" "${oneValueArgs}"
        "${multiValueArgs}" ${ARGN})

    foreach (dir IN ITEMS ${arg_HEADER_DIRS})
        string(FIND ${dir} "core" core_dir_pos)
        if(NOT ${core_dir_pos} EQUAL 0)
            message(FATAL_ERROR "Installed header directory should be relative to core/ dir")
        endif()
        string(REPLACE "core" "kagome" relative_path ${dir})
        get_filename_component(install_prefix ${relative_path} DIRECTORY)
        install(DIRECTORY ${dir}
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${install_prefix}
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
