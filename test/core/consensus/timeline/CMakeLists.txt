#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

addtest(block_executor_test
    block_executor_test.cpp
    )

addtest(slots_util_test
    slots_util_test.cpp
    transaction_pool_error
)
target_link_libraries(slots_util_test
    logger_for_tests
)

addtest(timeline_test
    timeline_test.cpp
)
target_link_libraries(timeline_test
    logger_for_tests
)
