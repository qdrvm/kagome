#
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
#

addtest(babe_test
    babe_test.cpp
    )
target_link_libraries(babe_test
    consensus
    network
    clock
    waitable_timer
    sr25519_types
    sr25519_provider
    logger_for_tests
    )

addtest(babe_config_repository_test
    babe_config_repository_test.cpp
    )
target_link_libraries(babe_config_repository_test
    consensus
    clock
    storage
    logger_for_tests
    )

addtest(threshold_util_test
    threshold_util_test.cpp
    )
target_link_libraries(threshold_util_test
    consensus
    )

addtest(block_executor_test
    block_executor_test.cpp
    )
target_link_libraries(block_executor_test
#    consensus
    timeline
    log_configurator
    blockchain
    transaction_pool_error
    )
