#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(impl)

add_library(transaction_pool
    impl/transaction_pool_impl.cpp
    )
target_link_libraries(transaction_pool
    outcome
    pool_moderator
    )
