#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(block_executor_test
    block_executor_test.cpp
    )
target_link_libraries(block_executor_test
    timeline
    log_configurator
    blockchain
    transaction_pool_error
    )
