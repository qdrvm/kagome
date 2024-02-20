#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/node
    )

add_subdirectory(core)
add_subdirectory(deps)
add_subdirectory(testutil)

if(EXTERNAL_PROJECT)
  add_executable(external_project external-project-test/src/main.cpp)
  set(external_project_include ${CMAKE_CURRENT_BINARY_DIR}/external_project_include)
  file(MAKE_DIRECTORY ${external_project_include})
  file(CREATE_LINK ${PROJECT_SOURCE_DIR}/core ${external_project_include}/kagome SYMBOLIC)
  target_include_directories(external_project PRIVATE ${external_project_include})
  include(external-project-test/link_libraries.cmake)
  external_project_link_libraries(external_project "")
endif()
