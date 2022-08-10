##
# Copyright Soramitsu Co., Ltd. All Rights Reserved.
# SPDX-License-Identifier: Apache-2.0
##

addtest(block_header_repository_test
    block_header_repository_test.cpp
    )
target_link_libraries(block_header_repository_test
    block_header_repository
    base_rocksdb_test
    hasher
    blockchain_common
    )

addtest(block_tree_test
    block_tree_test.cpp
    )
target_link_libraries(block_tree_test
    block_tree
    block_header_repository
    extrinsic_observer
    babe_digests_util
    logger_for_tests
    dummy_error
    )

addtest(block_storage_test
    block_storage_test.cpp
    )
target_link_libraries(block_storage_test
    block_storage
    logger_for_tests
    )

addtest(justification_storage_policy_test
        justification_storage_policy_test.cpp
        )
target_link_libraries(justification_storage_policy_test
        justification_storage_policy
        buffer
        scale::scale
        )
