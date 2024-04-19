#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

get_property(tests GLOBAL PROPERTY tests)
set(test_objects)
set(libtest_libraries)
foreach(test ${tests})
  list(APPEND test_objects $<TARGET_OBJECTS:${test}>)
  get_target_property(libraries ${test} LINK_LIBRARIES)
  list(APPEND libtest_libraries ${libraries})
endforeach()
list(SORT test_objects)
list(SORT libtest_libraries)
list(REMOVE_DUPLICATES libtest_libraries)
set(libtest_c ${CMAKE_BINARY_DIR}/libtest.c)
set(libtest_generate ${CMAKE_CURRENT_LIST_DIR}/libtest-generate.py)
# generate library with symbols imported by tests
add_custom_command(
  OUTPUT ${libtest_c}
  COMMAND python3 ${libtest_generate} ${libtest_c} ${test_objects}
  DEPENDS ${libtest_generate} ${tests}
  COMMAND_EXPAND_LISTS
)
add_library(libtest SHARED ${libtest_c})
target_link_libraries(libtest PUBLIC ${libtest_libraries})
