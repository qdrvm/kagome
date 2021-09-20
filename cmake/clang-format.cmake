file(GLOB_RECURSE
    ALL_CXX_SOURCE_FILES
    core/*.[ch]pp
    core/*.[ch]
    node/*.[ch]pp
    node/*.[ch]
    test/*.[ch]pp
    test/*.[ch]
    )

# Adding clang-format target if executable is found
if(NOT CLANG_FORMAT_BIN)
  foreach(CLANG_FORMAT_VERSION RANGE 20 7 -1)
    find_program(CLANG_FORMAT_BIN NAMES clang-format-${CLANG_FORMAT_VERSION})
    if(CLANG_FORMAT_BIN)
      message(STATUS "Found clang-format ver. ${CLANG_FORMAT_VERSION}")
      break()
    endif()
  endforeach()
  if(NOT CLANG_FORMAT_BIN)
    find_program(CLANG_FORMAT_BIN NAMES clang-format)
  endif()
endif()

if(CLANG_FORMAT_BIN)
  message(STATUS "Target clang-format enabled")
  add_custom_target(
      clang-format
      COMMAND "${CLANG_FORMAT_BIN}"
      -i
      ${ALL_CXX_SOURCE_FILES}
  )
endif()
