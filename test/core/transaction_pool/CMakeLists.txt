#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

addtest(pool_moderator_test
    pool_moderator_test.cpp
    )

addtest(transaction_pool_test
    transaction_pool_test.cpp
    )
target_link_libraries(transaction_pool_test
    logger_for_tests
    )
