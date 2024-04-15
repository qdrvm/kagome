#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

addtest(babe_test
    babe_test.cpp
    )
target_link_libraries(babe_test
    logger_for_tests
    )

addtest(babe_lottery_test
    babe_lottery_test.cpp
)
target_link_libraries(babe_lottery_test
    logger_for_tests
)

addtest(babe_block_validating_test
    babe_block_validator_test.cpp
)
target_link_libraries(babe_block_validating_test
    logger_for_tests
)

addtest(threshold_util_test
    threshold_util_test.cpp
    )
