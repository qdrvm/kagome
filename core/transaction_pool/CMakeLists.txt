#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(transaction_pool INTERFACE)
target_link_libraries(transaction_pool INTERFACE
    outcome
    pool_moderator
    logger
    )
