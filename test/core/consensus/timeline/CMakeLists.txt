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

addtest(slots_util_test
    slots_util_test.cpp
)
target_link_libraries(slots_util_test
    consensus
    clock
    storage
    logger_for_tests
)

addtest(timeline_test
    timeline_test.cpp
)
target_link_libraries(timeline_test
    consensus
    clock
    storage
    logger_for_tests
    network
    telemetry
)
