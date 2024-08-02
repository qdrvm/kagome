#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

addtest(parachain_test
    pvf_test.cpp
    assignments.cpp
    prospective_parachains.cpp
    cluster_test.cpp
    grid.cpp
    )

target_link_libraries(parachain_test
    validator_parachain
    log_configurator
    base_fs_test
    key_store
    logger
    )

if (CMAKE_SYSTEM_NAME STREQUAL Linux)
    target_sources(parachain_test PRIVATE secure_mode.cpp)
endif()
