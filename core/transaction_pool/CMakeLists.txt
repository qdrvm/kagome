#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

add_library(transaction_pool_error
    transaction_pool_error.cpp
    )
target_link_libraries(transaction_pool_error
    outcome
    )
