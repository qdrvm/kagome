#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

addtest(babe_test
    babe_test.cpp
    )
target_link_libraries(babe_test
    primitives
    consensus
    network
    clock
    waitable_timer
    sr25519_types
    sr25519_provider
    logger_for_tests
    )

addtest(babe_lottery_test
    babe_lottery_test.cpp
)
target_link_libraries(babe_lottery_test
    consensus
    logger_for_tests
)

addtest(babe_block_validating_test
    babe_block_validator_test.cpp
)
target_link_libraries(babe_block_validating_test
    consensus
    logger_for_tests
)

addtest(threshold_util_test
    threshold_util_test.cpp
    )
target_link_libraries(threshold_util_test
    consensus
    )
