#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(timeline)
add_subdirectory(babe)
add_subdirectory(grandpa)

addtest(beefy_test
    beefy_test.cpp
    )
target_link_libraries(beefy_test
    logger_for_tests
    network
    in_memory_storage
    )
