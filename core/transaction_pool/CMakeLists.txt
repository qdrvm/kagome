#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(pool_moderator
    impl/pool_moderator_impl.cpp
    )
target_link_libraries(pool_moderator
    Boost::boost
    blob
    )

add_library(transaction_pool_error
    transaction_pool_error.cpp
    )
target_link_libraries(transaction_pool_error
    outcome
    )

add_library(transaction_pool
    impl/transaction_pool_impl.cpp)
target_link_libraries(transaction_pool
    outcome
    pool_moderator
    logger
    blob
    transaction_pool_error
    block_header_repository
    metrics
    )
