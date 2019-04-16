function(disable_clang_tidy target)
  set_target_properties(${target} PROPERTIES
    C_CLANG_TIDY             ""
    CXX_CLANG_TIDY           ""
    )
endfunction()

function(addtest test_name)
  add_executable(${test_name} ${ARGN})
  addtest_part(${test_name} ${ARGN})
  target_link_libraries(${test_name}
    GTest::main
    GMock::main
    )
  add_test(
    NAME    ${test_name}
    COMMAND $<TARGET_FILE:${test_name}>
  )
  set_target_properties(${test_name} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test_bin
    ARCHIVE_OUTPUT_PATH      ${CMAKE_BINARY_DIR}/test_lib
    LIBRARY_OUTPUT_PATH      ${CMAKE_BINARY_DIR}/test_lib
    )
  disable_clang_tidy(${test_name})
endfunction()

function(addtest_part test_name)
  if(POLICY CMP0076)
    cmake_policy(SET CMP0076 NEW)
  endif()
  target_sources(${test_name} PUBLIC
    ${ARGN}
    )
  target_link_libraries(${test_name}
    GTest::gtest
    )
endfunction()


function(add_flag flag)
  check_cxx_compiler_flag(${flag} FLAG_${flag})
  if(FLAG_${flag} EQUAL 1)
    add_compile_options(${flag})
  endif()
endfunction()
