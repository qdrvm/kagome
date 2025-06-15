#
# Copyright Quadrivium LLC
# All Rights Reserved
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(polkadot_trie)
add_subdirectory(trie_storage)

addtest(direct_storage_test direct_storage_test.cpp)
target_link_libraries(direct_storage_test
    storage
    logger_for_tests
)
