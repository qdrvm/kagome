function(addtest test_name)
  add_executable(${test_name} ${ARGN})
  target_link_libraries(${test_name}
    GTest::main
    GTest::gtest
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
endfunction()
