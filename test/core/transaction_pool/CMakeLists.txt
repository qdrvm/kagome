#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(transaction_pool_test
    transaction_pool_test.cpp
    )
target_link_libraries(transaction_pool_test
    transaction_pool
    hexutil
    )
