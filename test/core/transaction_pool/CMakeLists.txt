#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

addtest(pool_moderator_test
    pool_moderator_test.cpp
    )
target_link_libraries(pool_moderator_test
    pool_moderator
    )

addtest(transaction_pool_test
    transaction_pool_test.cpp
    )
target_link_libraries(transaction_pool_test
    transaction_pool
    hexutil
    logger_for_tests
    )
