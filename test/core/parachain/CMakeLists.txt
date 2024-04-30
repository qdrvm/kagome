#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

addtest(pvf_test
    pvf_test.cpp
    )

if (CMAKE_SYSTEM_NAME STREQUAL Linux)
    target_sources(pvf_test PRIVATE secure_mode.cpp)
endif()

target_link_libraries(pvf_test
    validator_parachain
    log_configurator
    )

addtest(assignments_test
    assignments.cpp
    )
target_link_libraries(assignments_test
    key_store
    base_fs_test
    validator_parachain
    )

addtest(prospective_parachains_test
    prospective_parachains.cpp
    )

target_link_libraries(prospective_parachains_test
    validator_parachain
    log_configurator
    )
