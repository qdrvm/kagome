#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

addtest(block_header_repository_test
    block_header_repository_test.cpp
    )
target_link_libraries(block_header_repository_test
    base_rocksdb_test
    )

addtest(block_tree_test
    block_tree_test.cpp
    )
target_link_libraries(block_tree_test
    logger_for_tests
    dummy_error
    )

addtest(block_storage_test
    block_storage_test.cpp
    )
target_link_libraries(block_storage_test
    logger_for_tests
    )

addtest(justification_storage_policy_test
    justification_storage_policy_test.cpp
    )